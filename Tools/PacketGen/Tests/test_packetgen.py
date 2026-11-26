"""Tests of packetgen itself: how it reads the definitions and what it refuses (CA-03)."""

from __future__ import annotations

import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "Private"))

import packetgen  # noqa: E402  (the path above comes first)

_FIXTURE = Path(__file__).resolve().parent / "Fixture.xml"
_OPENMU = Path(__file__).resolve().parents[3] / "third_party" / "openmu"

_TEMPLATE = """<?xml version="1.0" encoding="utf-8"?>
<PacketDefinitions xmlns="http://www.munique.net/OpenMU/PacketDefinitions">
  <Structures>{structures}</Structures>
  <Packets>{packets}</Packets>
</PacketDefinitions>
"""


def _definitions(packets: str, structures: str = "") -> packetgen.DefinitionFile:
    with tempfile.TemporaryDirectory() as folder:
        path = Path(folder) / "Test.xml"
        path.write_text(_TEMPLATE.format(packets=packets, structures=structures), encoding="utf-8")
        return packetgen.read_definitions(path)


def _packet(fields: str, header: str = "C1Header", sub_code: str = "") -> str:
    return (f"<Packet><HeaderType>{header}</HeaderType><Code>10</Code>{sub_code}<Name>Test</Name>"
            f"<Direction>ServerToClient</Direction><SentWhen /><CausedReaction /><Fields>{fields}</Fields></Packet>")


class NamesTest(unittest.TestCase):
    def test_names_become_pascal_case_without_underscores(self) -> None:
        self.assertEqual(packetgen.cpp_name("__RivalCount"), "RivalCount")
        self.assertEqual(packetgen.cpp_name("FailedBy_"), "FailedBy")
        self.assertEqual(packetgen.cpp_name("npcs"), "Npcs")
        self.assertEqual(packetgen.cpp_name("3D"), "Value3D")

    def test_a_name_without_letters_is_refused(self) -> None:
        with self.assertRaises(packetgen.DefinitionError):
            packetgen.cpp_name("__")

    def test_numbers_are_hexadecimal_only_with_0x(self) -> None:
        self.assertEqual(packetgen.parse_number("0x28"), 40)
        self.assertEqual(packetgen.parse_number("01"), 1)


class SizesTest(unittest.TestCase):
    def test_the_fixture_has_the_sizes_the_tests_of_the_generated_code_expect(self) -> None:
        definitions = packetgen.read_definitions(_FIXTURE)
        structures = {item.name: item for item in definitions.structures}
        sizes = {packet.name: packetgen.minimum_length(packet, structures) for packet in definitions.packets}
        self.assertEqual(sizes, {"EveryField": 28, "PointList": 5, "Fixed": 4})
        self.assertEqual(packetgen.structure_size(structures["Point"], structures), 3)

    def test_the_minimum_holds_the_subcode_even_when_it_is_a_field(self) -> None:
        definitions = _definitions(_packet("", header="C2HeaderWithSubCode"))
        self.assertEqual(packetgen.minimum_length(definitions.packets[0], {}), 5)

    def test_structures_come_after_the_structures_they_list(self) -> None:
        inner = "<Structure><Name>Inner</Name><Description /><Fields /></Structure>"
        outer = ("<Structure><Name>Outer</Name><Description /><Fields><Field><Index>0</Index>"
                 "<Type>Structure[]</Type><TypeName>Inner</TypeName><Name>Items</Name></Field></Fields></Structure>")
        definitions = _definitions("", structures=outer + inner)
        ordered = packetgen.ordered_structures(definitions.structures)
        self.assertEqual([item.name for item in ordered], ["Inner", "Outer"])


class RefusalsTest(unittest.TestCase):
    def test_an_unknown_field_type_is_refused(self) -> None:
        with self.assertRaisesRegex(packetgen.DefinitionError, "unknown field type 'Triple'"):
            _definitions(_packet("<Field><Index>3</Index><Type>Triple</Type><Name>A</Name></Field>"))

    def test_a_subcode_without_room_in_the_header_is_refused(self) -> None:
        with self.assertRaisesRegex(packetgen.DefinitionError, "has a subcode"):
            _definitions(_packet("", sub_code="<SubCode>01</SubCode>"))

    def test_a_field_larger_than_its_bits_is_refused(self) -> None:
        definitions = _definitions(_packet(
            "<Field><Index>3</Index><LeftShifted>6</LeftShifted><Type>Byte</Type><Name>A</Name>"
            "<Length>4</Length></Field>"))
        with self.assertRaisesRegex(packetgen.DefinitionError, "does not fit in its 8 bits"):
            packetgen.generate_header(definitions, "test", None, None)

    def test_two_definitions_with_one_name_are_refused(self) -> None:
        definitions = _definitions(_packet("") + _packet(""))
        with self.assertRaisesRegex(packetgen.DefinitionError, "two definitions are named Test"):
            packetgen.generate_header(definitions, "test", None, None)


class OpenMuFilesTest(unittest.TestCase):
    def test_every_file_of_openmu_generates(self) -> None:
        common = packetgen.read_definitions(_OPENMU / "CommonEnums.xml")
        scope = packetgen.file_scope(common, "common", None)
        counts = {}
        for name, peer in (("ConnectServer", None), ("ClientToServer", "ServerToClient"),
                           ("ServerToClient", "ClientToServer"), ("ChatServer", None)):
            file_scope = packetgen.Scope(dict(scope.enums), dict(scope.structures), dict(scope.structure_names),
                                         dict(scope.enum_values))
            if peer is not None:
                file_scope = packetgen.with_peer_values(
                    file_scope, packetgen.read_definitions(_OPENMU / f"{peer}Packets.xml"))
            definitions = packetgen.read_definitions(_OPENMU / f"{name}Packets.xml")
            header = packetgen.generate_header(definitions, "test", file_scope, None)
            self.assertIn("struct", header)
            counts[name] = len(definitions.packets)
        # The plan counts 391 packets from the server and 204 from the client (specs/0001-rede-ate-o-mundo/plan.md).
        self.assertEqual(counts, {"ConnectServer": 11, "ClientToServer": 204, "ServerToClient": 391, "ChatServer": 7})


if __name__ == "__main__":
    unittest.main()

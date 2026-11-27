"""Tests of packetgen itself: how it reads the definitions and what it refuses (CA-03)."""

from __future__ import annotations

import contextlib
import io
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
            packetgen.generate_headers(definitions, "test", None, None, "Test")

    def test_two_definitions_with_one_name_are_refused(self) -> None:
        definitions = _definitions(_packet("") + _packet(""))
        with self.assertRaisesRegex(packetgen.DefinitionError, "two definitions are named Test"):
            packetgen.generate_headers(definitions, "test", None, None, "Test")

    def test_a_packet_named_like_the_shared_header_is_refused(self) -> None:
        definitions = _definitions(_packet("").replace("<Name>Test</Name>", "<Name>Types</Name>"))
        with self.assertRaisesRegex(packetgen.DefinitionError, "the packet Types would have the file"):
            packetgen.generate_headers(definitions, "test", None, None, "Test")

    def test_two_packets_whose_files_differ_only_in_case_are_refused(self) -> None:
        definitions = _definitions(_packet("") + _packet("").replace("<Name>Test</Name>", "<Name>TEST</Name>"))
        with self.assertRaisesRegex(packetgen.DefinitionError, "the packet TEST would have the file"):
            packetgen.generate_headers(definitions, "test", None, None, "Test")


def _openmu_files() -> dict[str, tuple[packetgen.DefinitionFile, list[packetgen.GeneratedFile],
                                       list[packetgen.GeneratedFile]]]:
    """The definitions, the headers and the tests of each file of OpenMU, as the build generates them."""
    common = packetgen.read_definitions(_OPENMU / "CommonEnums.xml")
    scope = packetgen.file_scope(common, "common", None)
    files = {}
    for name, peer in (("ConnectServer", None), ("ClientToServer", "ServerToClient"),
                       ("ServerToClient", "ClientToServer"), ("ChatServer", None)):
        file_scope = packetgen.Scope(dict(scope.enums), dict(scope.structures), dict(scope.structure_names),
                                     dict(scope.enum_values))
        if peer is not None:
            file_scope = packetgen.with_peer_values(
                file_scope, packetgen.read_definitions(_OPENMU / f"{peer}Packets.xml"))
        definitions = packetgen.read_definitions(_OPENMU / f"{name}Packets.xml")
        include_dir = f"Mu/Protocol/Packets/{name}"
        headers = packetgen.generate_headers(definitions, "test", file_scope, "Mu/Protocol/Packets/Common/Types.hpp",
                                             include_dir)
        tests = packetgen.generate_tests(definitions, "test", include_dir, f"Tests/{name}PacketsTest")
        files[name] = (definitions, headers, tests)
    return files


class OpenMuFilesTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.files = _openmu_files()

    def test_every_file_of_openmu_generates_one_header_for_each_packet(self) -> None:
        counts = {name: len(definitions.packets) for name, (definitions, _, _) in self.files.items()}
        # The plan counts 391 packets from the server and 204 from the client (specs/0001-rede-ate-o-mundo/plan.md).
        self.assertEqual(counts, {"ConnectServer": 11, "ClientToServer": 204, "ServerToClient": 391, "ChatServer": 7})
        for name, (definitions, headers, _) in self.files.items():
            # Types.hpp, then each packet.
            self.assertEqual(len(headers), len(definitions.packets) + 1, name)
            self.assertEqual(headers[0].path, f"Mu/Protocol/Packets/{name}/Types.hpp")

    def test_no_header_includes_another_packet(self) -> None:
        # Proposal P-0016: a packet header includes only the shared types of its file, so the code that reads one
        # packet never compiles the others.
        for name, (_, headers, _) in self.files.items():
            for header in headers:
                packet_includes = [line for line in header.text.splitlines()
                                   if line.startswith("#include \"Mu/Protocol/Packets/") and "/Types.hpp\"" not in line]
                self.assertEqual(packet_includes, [], header.path)

    def test_the_generated_tests_cover_each_packet_once_in_small_files(self) -> None:
        for name, (definitions, _, tests) in self.files.items():
            tested = [line.split("CheckPacketShape<::test::", 1)[1].split(">", 1)[0]
                      for test in tests for line in test.text.splitlines() if "CheckPacketShape<::test::" in line]
            self.assertEqual(sorted(tested), sorted(packetgen.cpp_name(packet.name) for packet in definitions.packets),
                             name)
            for test in tests:
                self.assertLessEqual(test.text.count("TEST_CASE("), 40, test.path)
                # A file of tests includes only the headers of the packets it tests.
                includes = [line for line in test.text.splitlines() if line.startswith("#include \"Mu/")]
                self.assertEqual(len(includes), test.text.count("TEST_CASE("), test.path)

    def test_the_list_names_the_files_the_generator_writes(self) -> None:
        with tempfile.TemporaryDirectory() as folder:
            arguments = ["--xml", str(_OPENMU / "ConnectServerPackets.xml"), "--namespace", "test", "--output-dir",
                         folder, "--include-dir", "Packets/ConnectServer", "--tests-prefix", "Tests/ConnectTest"]
            listed = io.StringIO()
            with contextlib.redirect_stdout(listed):
                self.assertEqual(packetgen.main([*arguments, "--list"]), 0)
            self.assertEqual(list(Path(folder).rglob("*")), [])
            self.assertEqual(packetgen.main(arguments), 0)
            written = sorted(path.as_posix() for path in Path(folder).rglob("*") if path.is_file())
            named = sorted(line.split(" ", 1)[1] for line in listed.getvalue().splitlines())
            self.assertEqual(named, written)
            self.assertEqual(len(named), 11 + 1 + 1)


if __name__ == "__main__":
    unittest.main()

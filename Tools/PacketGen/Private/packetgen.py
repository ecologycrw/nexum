"""packetgen: turns the packet definitions of OpenMU (XML, MIT) into C++ that reads and writes every packet.

For each XML file it writes, in a folder of its own (docs/adr/0034-packetgen-em-python-no-build.md):
- Types.hpp, with the enums and structures that the packets of the file share;
- one header for each packet, named after it, with a struct holding the bytes that never change (the shape), the
  enums and structures of the packet, a View that checks the size before giving access to the fields, and a Writer
  that builds the packet in a buffer. A packet header includes only Types.hpp, so the code that reads a packet
  compiles that packet alone, and no header ever includes every packet (proposal P-0016);
- doctest tests split in files of a few dozen packets each, which check, for each packet, the fixed bytes, the
  length and the refusal of a shorter packet.

Every field is read by offset, with the byte order and the bit position that the XML declares. The generated code
uses the pieces of Mu/Protocol/PacketFields.hpp. Python 3 with the standard library only; exceptions are allowed
here, because this is a tool (INV-14).

Usage:
    packetgen.py --xml FILE --namespace NS --output-dir DIR --include-dir Mu/Protocol/Packets/Name
                 [--tests-prefix Tests/NameTest]
                 [--common COMMON.xml --common-namespace NS --common-include INCLUDE] [--peer FILE...] [--list]

With --list it writes nothing and prints, one per line, "header PATH" or "tests PATH" for each file it would write,
so the build knows the files before it runs the generator.
"""

from __future__ import annotations

import argparse
import re
import sys
import xml.etree.ElementTree as ElementTree
from dataclasses import dataclass, field
from pathlib import Path

_NS = "{http://www.munique.net/OpenMU/PacketDefinitions}"

# The header structures of PacketHeaders.xml: the first byte, the size of the header and whether a subcode follows.
_HEADERS = {
    "C1Header": ("C1", False), "C1HeaderWithSubCode": ("C1", True),
    "C2Header": ("C2", False), "C2HeaderWithSubCode": ("C2", True),
    "C3Header": ("C3", False), "C3HeaderWithSubCode": ("C3", True),
    "C4Header": ("C4", False), "C4HeaderWithSubCode": ("C4", True),
}
_HEADER_SIZE = {"C1": 2, "C2": 3, "C3": 2, "C4": 3}

# Each fixed-size type of the XML: its size in bytes, the C++ type, and how it is read and written.
_NUMBERS = {
    "Byte": (1, "std::uint8_t", "::mu::protocol::ReadByte(m_bytes, {o})", "::mu::protocol::WriteByte(m_bytes, {o}, {v})"),
    "ShortLittleEndian": (2, "std::uint16_t", "::mu::protocol::ReadLittleEndian<std::uint16_t>(m_bytes, {o})",
                          "::mu::protocol::WriteLittleEndian<std::uint16_t>(m_bytes, {o}, {v})"),
    "ShortBigEndian": (2, "std::uint16_t", "::mu::protocol::ReadBigEndian<std::uint16_t>(m_bytes, {o})",
                       "::mu::protocol::WriteBigEndian<std::uint16_t>(m_bytes, {o}, {v})"),
    "IntegerLittleEndian": (4, "std::uint32_t", "::mu::protocol::ReadLittleEndian<std::uint32_t>(m_bytes, {o})",
                            "::mu::protocol::WriteLittleEndian<std::uint32_t>(m_bytes, {o}, {v})"),
    "IntegerBigEndian": (4, "std::uint32_t", "::mu::protocol::ReadBigEndian<std::uint32_t>(m_bytes, {o})",
                         "::mu::protocol::WriteBigEndian<std::uint32_t>(m_bytes, {o}, {v})"),
    "LongLittleEndian": (8, "std::uint64_t", "::mu::protocol::ReadLittleEndian<std::uint64_t>(m_bytes, {o})",
                         "::mu::protocol::WriteLittleEndian<std::uint64_t>(m_bytes, {o}, {v})"),
    "LongBigEndian": (8, "std::uint64_t", "::mu::protocol::ReadBigEndian<std::uint64_t>(m_bytes, {o})",
                      "::mu::protocol::WriteBigEndian<std::uint64_t>(m_bytes, {o}, {v})"),
    "Float": (4, "float", "::mu::protocol::ReadFloat(m_bytes, {o})", "::mu::protocol::WriteFloat(m_bytes, {o}, {v})"),
    "Double": (8, "double", "::mu::protocol::ReadDouble(m_bytes, {o})", "::mu::protocol::WriteDouble(m_bytes, {o}, {v})"),
}

# Names the generated classes already use for themselves.
_RESERVED = {"Read", "Bytes", "Create", "Start", "View", "Writer"}

# The header with the enums and structures that the packets of a file share; no packet may have its name.
_TYPES = "Types"

# How many packets each file of generated tests holds: small files compile with little memory and in parallel.
_PACKETS_PER_TEST_FILE = 40

_IDENTIFIER = re.compile(r"[^A-Za-z0-9]")


class DefinitionError(Exception):
    """The XML does not describe something the generator understands."""


def cpp_name(name: str) -> str:
    """A C++ name in PascalCase: the underscores at the ends go away, and a name never starts with a digit."""
    cleaned = _IDENTIFIER.sub("", name.strip("_"))
    if not cleaned:
        raise DefinitionError(f"the name '{name}' has no letters")
    if cleaned[0].isdigit():
        cleaned = "Value" + cleaned
    return cleaned[0].upper() + cleaned[1:]


@dataclass
class EnumValue:
    name: str
    value: int


@dataclass
class EnumDefinition:
    name: str
    values: list[EnumValue]


@dataclass
class FieldDefinition:
    index: int
    kind: str
    name: str
    type_name: str | None = None
    length: int | None = None
    shift: int | None = None
    default: str | None = None
    count_field: str | None = None
    custom_indexer: bool = False


@dataclass
class StructureDefinition:
    name: str
    length: int | None
    fields: list[FieldDefinition]


@dataclass
class PacketDefinition:
    name: str
    header: str
    code: int
    sub_code: int | None
    length: int | None
    direction: str
    fields: list[FieldDefinition]
    structures: list[StructureDefinition] = field(default_factory=list)
    enums: list[EnumDefinition] = field(default_factory=list)


@dataclass
class DefinitionFile:
    source: str
    packets: list[PacketDefinition]
    structures: list[StructureDefinition]
    enums: list[EnumDefinition]


# Reading the XML.

def _text(element: ElementTree.Element, tag: str) -> str | None:
    found = element.find(_NS + tag)
    return None if found is None or found.text is None else found.text.strip()


def parse_number(text: str) -> int:
    """A number of the XML: hexadecimal with 0x, decimal otherwise (leading zeros allowed)."""
    return int(text, 16) if text.lower().startswith("0x") else int(text, 10)


def _number(element: ElementTree.Element, tag: str) -> int | None:
    text = _text(element, tag)
    return None if text is None else parse_number(text)


def _children(element: ElementTree.Element, group: str, item: str) -> list[ElementTree.Element]:
    found = element.find(_NS + group)
    return [] if found is None else found.findall(_NS + item)


def _read_field(element: ElementTree.Element) -> FieldDefinition:
    kind = _text(element, "Type") or ""
    if kind not in _NUMBERS and kind not in ("Boolean", "String", "Binary", "Enum", "Structure[]"):
        raise DefinitionError(f"unknown field type '{kind}'")
    index = _number(element, "Index")
    if index is None:
        raise DefinitionError("a field has no index")
    return FieldDefinition(
        index=index,
        kind=kind,
        name=_text(element, "Name") or f"Field{index}",
        type_name=_text(element, "TypeName"),
        length=_number(element, "Length"),
        shift=_number(element, "LeftShifted"),
        default=_text(element, "DefaultValue"),
        count_field=_text(element, "ItemCountField"),
        custom_indexer=(_text(element, "UseCustomIndexer") or "").lower() == "true",
    )


def _read_enum(element: ElementTree.Element) -> EnumDefinition:
    values = [
        EnumValue(_text(value, "Name") or "", parse_number(_text(value, "Value") or "0"))
        for value in _children(element, "Values", "EnumValue")
    ]
    return EnumDefinition(_text(element, "Name") or "", values)


def _read_structure(element: ElementTree.Element) -> StructureDefinition:
    return StructureDefinition(
        name=_text(element, "Name") or "",
        length=_number(element, "Length"),
        fields=[_read_field(item) for item in _children(element, "Fields", "Field")],
    )


def _read_packet(element: ElementTree.Element) -> PacketDefinition:
    name = _text(element, "Name") or ""
    header = _text(element, "HeaderType") or ""
    if header not in _HEADERS:
        raise DefinitionError(f"the packet {name} has an unknown header '{header}'")
    sub_code = _text(element, "SubCode")
    if sub_code is not None and not _HEADERS[header][1]:
        raise DefinitionError(f"the packet {name} has a subcode, but its header {header} has none")
    return PacketDefinition(
        name=name,
        header=header,
        code=int(_text(element, "Code") or "0", 16),
        sub_code=None if sub_code is None else int(sub_code, 16),
        length=_number(element, "Length"),
        direction=_text(element, "Direction") or "",
        fields=[_read_field(item) for item in _children(element, "Fields", "Field")],
        structures=[_read_structure(item) for item in _children(element, "Structures", "Structure")],
        enums=[_read_enum(item) for item in _children(element, "Enums", "Enum")],
    )


def read_definitions(path: Path) -> DefinitionFile:
    root = ElementTree.parse(path).getroot()
    if root.tag != _NS + "PacketDefinitions":
        raise DefinitionError(f"{path.name} is not a file of packet definitions")
    return DefinitionFile(
        source=path.name,
        packets=[_read_packet(item) for item in _children(root, "Packets", "Packet")],
        structures=[_read_structure(item) for item in _children(root, "Structures", "Structure")],
        enums=[_read_enum(item) for item in _children(root, "Enums", "Enum")],
    )


# Sizes.

def field_size(item: FieldDefinition, structures: dict[str, StructureDefinition]) -> int | None:
    """The bytes a field takes, or None when it goes to the end of the packet or its size depends on the data."""
    if item.kind in _NUMBERS:
        return _NUMBERS[item.kind][0]
    if item.kind in ("Boolean", "Enum"):
        return 1
    if item.kind in ("String", "Binary"):
        return item.length
    if item.kind == "Structure[]":
        if item.length is None or item.custom_indexer:
            return None
        size = structure_size(structures[item.type_name or ""], structures)
        return None if size is None else size * item.length
    raise DefinitionError(f"unknown field type '{item.kind}'")


def structure_size(structure: StructureDefinition, structures: dict[str, StructureDefinition]) -> int | None:
    """The size of one structure, or None when it depends on the data (a list or a field that goes to the end)."""
    end = 0
    for item in structure.fields:
        size = field_size(item, structures)
        if size is None:
            return None
        end = max(end, item.index + size)
    return max(end, structure.length or 0)


def structure_minimum(structure: StructureDefinition, structures: dict[str, StructureDefinition]) -> int:
    """The smallest structure that holds every field with a fixed size; the same as its size when it is fixed."""
    end = 0
    for item in structure.fields:
        end = max(end, item.index + (field_size(item, structures) or 0))
    return max(end, structure.length or 0)


def ordered_structures(structures: list[StructureDefinition]) -> list[StructureDefinition]:
    """The structures with each one after the structures it lists, because C++ needs a type before its use."""
    by_name = {structure.name: structure for structure in structures}
    ordered: list[StructureDefinition] = []
    visiting: set[str] = set()

    def visit(structure: StructureDefinition) -> None:
        if structure in ordered:
            return
        if structure.name in visiting:
            raise DefinitionError(f"the structure {structure.name} contains itself")
        visiting.add(structure.name)
        for item in structure.fields:
            if item.kind == "Structure[]" and item.type_name in by_name:
                visit(by_name[item.type_name])
        visiting.discard(structure.name)
        ordered.append(structure)

    for structure in structures:
        visit(structure)
    return ordered


def minimum_length(packet: PacketDefinition, structures: dict[str, StructureDefinition]) -> int:
    """The smallest packet that holds the header, the code, the subcode and every field with a fixed size."""
    kind, with_sub_code = _HEADERS[packet.header]
    end = _HEADER_SIZE[kind] + 1 + (1 if with_sub_code else 0)
    for item in packet.fields:
        size = field_size(item, structures)
        end = max(end, item.index + (size or 0))
    return end


# Writing the C++ code.

@dataclass
class Scope:
    """What a name in the XML can refer to, from the closest to the farthest: the packet, the file, the common file."""
    enums: dict[str, str]
    structures: dict[str, StructureDefinition]
    structure_names: dict[str, str]
    enum_values: dict[str, dict[str, int]]


def _bits(item: FieldDefinition, bits_of_type: int) -> tuple[int, int] | None:
    """The shift and the length of a field that takes only part of its bytes, or None for a whole field."""
    shift = item.shift or 0
    length = item.length if item.kind not in ("String", "Binary", "Structure[]") else None
    if item.kind == "Boolean":
        length = 1
    elif shift == 0 and (length is None or length == bits_of_type):
        return None
    elif length is None:
        length = bits_of_type - shift
    if shift + length > bits_of_type or length <= 0:
        raise DefinitionError(f"the field {item.name} does not fit in its {bits_of_type} bits")
    return (shift, length)


def _default_value(item: FieldDefinition, scope: Scope) -> str | None:
    if item.default is None:
        return None
    text = item.default
    if text.lower() in ("true", "false"):
        return text.lower()
    if re.fullmatch(r"0x[0-9A-Fa-f]+|\d+", text):
        return str(parse_number(text))
    parts = text.split(".")
    for enum in (".".join(parts[:-1]), parts[-2] if len(parts) >= 2 else ""):
        if enum in scope.enum_values and parts[-1] in scope.enum_values[enum]:
            return str(scope.enum_values[enum][parts[-1]])
    raise DefinitionError(f"cannot read the default value '{text}' of the field {item.name}")


class CodeWriter:
    def __init__(self) -> None:
        self.lines: list[str] = []
        self.depth = 0

    def line(self, text: str = "") -> None:
        self.lines.append(("    " * self.depth + text) if text else "")

    def open(self, text: str) -> None:
        self.line(text)
        self.depth += 1

    def close(self, text: str = "}") -> None:
        self.depth -= 1
        self.line(text)

    def text(self) -> str:
        return "\n".join(self.lines) + "\n"


def _write_enum(out: CodeWriter, enum: EnumDefinition) -> None:
    # The schema says a value is one byte, but a few enums have larger values; they get two bytes.
    underlying = "std::uint8_t" if all(value.value <= 0xFF for value in enum.values) else "std::uint16_t"
    out.open(f"enum class {cpp_name(enum.name)} : {underlying} {{")
    seen: set[str] = set()
    for value in enum.values:
        name = cpp_name(value.name)
        while name in seen:
            name += "Again"
        seen.add(name)
        out.line(f"{name} = {value.value},")
    out.close("};")


class FieldWriter:
    """Writes the getter of the View and the setter of the Writer for each field of a packet or of a structure."""

    def __init__(self, scope: Scope, owner: str, end_is_open: bool) -> None:
        self.scope = scope
        self.owner = owner
        self.end_is_open = end_is_open  # a packet can have fields that go to its end; a fixed structure cannot

    def _enum_type(self, item: FieldDefinition) -> str:
        if item.type_name is not None and "." in item.type_name and item.type_name not in self.scope.enums:
            # An enum of another file ("ServerToClient.GuildMemberRole"): the files use each other, so the field
            # keeps the raw byte instead of including the other header.
            return "std::uint8_t"
        if item.type_name not in self.scope.enums:
            raise DefinitionError(f"the field {item.name} uses the unknown enum {item.type_name}")
        return self.scope.enums[item.type_name]

    def getters(self, out: CodeWriter, item: FieldDefinition, names: dict[str, str]) -> None:
        name = names[item.name]
        offset = item.index
        if item.kind in _NUMBERS or item.kind in ("Boolean", "Enum"):
            self._number_getter(out, item, name, offset)
        elif item.kind == "String":
            out.line(f"[[nodiscard]] ::mu::protocol::PacketText {name}() const noexcept {{ "
                     f"return ::mu::protocol::PacketText({self._span(item, offset)}); }}")
        elif item.kind == "Binary":
            out.line(f"[[nodiscard]] std::span<const std::byte> {name}() const noexcept {{ "
                     f"return {self._span(item, offset)}; }}")
        else:
            self._list_getters(out, item, name, offset, names)

    def setters(self, out: CodeWriter, item: FieldDefinition, names: dict[str, str]) -> None:
        name = names[item.name]
        offset = item.index
        if item.kind in _NUMBERS or item.kind in ("Boolean", "Enum"):
            self._number_setter(out, item, name, offset)
        elif item.kind == "String":
            out.line(f"[[nodiscard]] ::nexum::core::Result<void> Set{name}(std::string_view text) const {{ "
                     f"return ::mu::protocol::WriteText({self._span(item, offset)}, text); }}")
        elif item.kind == "Binary":
            out.line(f"[[nodiscard]] ::nexum::core::Result<void> Set{name}(std::span<const std::byte> data) const {{ "
                     f"return ::mu::protocol::WriteBinary({self._span(item, offset)}, data); }}")
        else:
            self._list_setters(out, item, name, offset)

    def _span(self, item: FieldDefinition, offset: int) -> str:
        if item.length is not None:
            return f"m_bytes.subspan({offset}, {item.length})"
        return f"m_bytes.subspan({offset})"

    def _number_parts(self, item: FieldDefinition) -> tuple[str, str, str, tuple[int, int] | None]:
        kind = "Byte" if item.kind in ("Boolean", "Enum") else item.kind
        size, cpp_type, read, write = _NUMBERS[kind]
        bits = _bits(item, size * 8) if kind not in ("Float", "Double") else None
        return cpp_type, read, write, bits

    def _number_getter(self, out: CodeWriter, item: FieldDefinition, name: str, offset: int) -> None:
        cpp_type, read, _, bits = self._number_parts(item)
        value = read.format(o=offset)
        if bits is not None:
            value = f"::mu::protocol::TakeBits<{cpp_type}>({value}, {bits[0]}, {bits[1]})"
        if item.kind == "Boolean":
            out.line(f"[[nodiscard]] bool {name}() const noexcept {{ return {value} != 0; }}")
        elif item.kind == "Enum":
            enum_type = self._enum_type(item)
            out.line(f"[[nodiscard]] {enum_type} {name}() const noexcept {{ return static_cast<{enum_type}>({value}); }}")
        else:
            out.line(f"[[nodiscard]] {cpp_type} {name}() const noexcept {{ return {value}; }}")

    def _number_setter(self, out: CodeWriter, item: FieldDefinition, name: str, offset: int) -> None:
        cpp_type, read, write, bits = self._number_parts(item)
        if item.kind == "Boolean":
            parameter, value = "bool value", f"static_cast<{cpp_type}>(value ? 1 : 0)"
        elif item.kind == "Enum":
            parameter, value = f"{self._enum_type(item)} value", f"static_cast<{cpp_type}>(value)"
        else:
            parameter, value = f"{cpp_type} value", "value"
        if bits is not None:
            value = f"::mu::protocol::PutBits<{cpp_type}>({read.format(o=offset)}, {bits[0]}, {bits[1]}, {value})"
        out.line(f"void Set{name}({parameter}) const noexcept {{ {write.format(o=offset, v=value)}; }}")

    def _list_parts(self, item: FieldDefinition) -> tuple[StructureDefinition | None, int | None]:
        if item.custom_indexer:
            return None, None  # OpenMU reads these lists with code of its own: only the bytes are given
        structure = self.scope.structures.get(item.type_name or "")
        if structure is None:
            raise DefinitionError(f"the field {item.name} uses the unknown structure {item.type_name}")
        size = None if item.custom_indexer else structure_size(structure, self.scope.structures)
        return structure, size

    def _count_expression(self, item: FieldDefinition, names: dict[str, str], size: int) -> str:
        if item.count_field is not None:
            if item.count_field not in names:
                raise DefinitionError(f"the field {item.name} counts with the unknown field {item.count_field}")
            return f"static_cast<std::size_t>({names[item.count_field]}())"
        if item.length is not None:
            return str(item.length)
        return f"(m_bytes.size() - std::min<std::size_t>(m_bytes.size(), {item.index})) / {size}"

    def _list_getters(self, out: CodeWriter, item: FieldDefinition, name: str, offset: int,
                      names: dict[str, str]) -> None:
        structure, size = self._list_parts(item)
        out.line(f"[[nodiscard]] std::span<const std::byte> {name}Bytes() const noexcept {{ "
                 f"return m_bytes.subspan(std::min<std::size_t>(m_bytes.size(), {offset})); }}")
        if size is None or structure is None:
            return
        element = self.scope.structure_names[structure.name]
        out.line(f"[[nodiscard]] std::size_t {name}Count() const noexcept {{ "
                 f"return {self._count_expression(item, names, size)}; }}")
        out.open(f"[[nodiscard]] ::nexum::core::Result<{element}::View> {name}(std::size_t index) const {{")
        out.line(f"const std::size_t start = {offset} + (index * {size});")
        out.line(f"const auto rest = m_bytes.subspan(std::min<std::size_t>(m_bytes.size(), start));")
        out.line(f"return {element}::View::Read(rest.first(std::min<std::size_t>(rest.size(), {size})));")
        out.close()

    def _list_setters(self, out: CodeWriter, item: FieldDefinition, name: str, offset: int) -> None:
        structure, size = self._list_parts(item)
        out.line(f"[[nodiscard]] std::span<std::byte> {name}Bytes() const noexcept {{ "
                 f"return m_bytes.subspan(std::min<std::size_t>(m_bytes.size(), {offset})); }}")
        if size is None or structure is None:
            return
        element = self.scope.structure_names[structure.name]
        out.open(f"[[nodiscard]] ::nexum::core::Result<{element}::Writer> {name}(std::size_t index) const {{")
        out.line(f"const std::size_t start = {offset} + (index * {size});")
        out.line(f"const auto rest = m_bytes.subspan(std::min<std::size_t>(m_bytes.size(), start));")
        out.line(f"return {element}::Writer::Start(rest.first(std::min<std::size_t>(rest.size(), {size})));")
        out.close()


def _field_names(fields: list[FieldDefinition], owner: str) -> dict[str, str]:
    names: dict[str, str] = {}
    used: set[str] = set()
    for item in fields:
        name = cpp_name(item.name)
        if name in _RESERVED:
            name += "Field"
        while name in used:
            name += "Again"
        used.add(name)
        names[item.name] = name
    return names


def _write_defaults(out: CodeWriter, fields: list[FieldDefinition], names: dict[str, str], scope: Scope) -> None:
    for item in fields:
        value = _default_value(item, scope)
        if value is None:
            continue
        name = names[item.name]
        if item.kind == "Enum":
            enum_type = scope.enums.get(item.type_name or "", "std::uint8_t")
            out.line(f"writer.Set{name}(static_cast<{enum_type}>({value}));")
        elif item.kind == "Boolean":
            out.line(f"writer.Set{name}({value});")
        else:
            cpp_type = _NUMBERS[item.kind][1]
            out.line(f"writer.Set{name}(static_cast<{cpp_type}>({value}));")


def _write_structure(out: CodeWriter, structure: StructureDefinition, scope: Scope) -> None:
    name = cpp_name(structure.name)
    size = structure_size(structure, scope.structures)
    names = _field_names(structure.fields, name)
    fields = FieldWriter(scope, name, end_is_open=size is None)
    out.open(f"struct {name} {{")
    out.line(f"static constexpr std::string_view kName = \"{structure.name}\";")
    out.line(f"// The bytes of one {structure.name}; 0 when the size depends on the data.")
    out.line(f"static constexpr std::size_t kSize = {size or 0};")
    out.line(f"// The smallest {structure.name}: every field with a fixed place fits in it.")
    out.line(f"static constexpr std::size_t kMinimumSize = {structure_minimum(structure, scope.structures)};")
    out.line()
    out.open("class View {")
    out.line("public:")
    out.open("[[nodiscard]] static ::nexum::core::Result<View> Read(std::span<const std::byte> bytes) {")
    out.line("if (auto checked = ::mu::protocol::CheckStructure(bytes, kName, kMinimumSize); !checked) {")
    out.line("    return ::tl::unexpected(checked.error());")
    out.line("}")
    out.line("return View(bytes);")
    out.close()
    out.line("[[nodiscard]] std::span<const std::byte> Bytes() const noexcept { return m_bytes; }")
    for item in structure.fields:
        fields.getters(out, item, names)
    out.line()
    out.line("private:")
    out.line("explicit View(std::span<const std::byte> bytes) noexcept : m_bytes(bytes) {}")
    out.line("std::span<const std::byte> m_bytes;")
    out.close("};")
    out.line()
    out.open("class Writer {")
    out.line("public:")
    out.open("[[nodiscard]] static ::nexum::core::Result<Writer> Start(std::span<std::byte> bytes) {")
    out.line("if (auto checked = ::mu::protocol::CheckStructure(bytes, kName, kMinimumSize); !checked) {")
    out.line("    return ::tl::unexpected(checked.error());")
    out.line("}")
    out.line("Writer writer(bytes);")
    _write_defaults(out, structure.fields, names, scope)
    out.line("return writer;")
    out.close()
    out.line("[[nodiscard]] std::span<std::byte> Bytes() const noexcept { return m_bytes; }")
    for item in structure.fields:
        fields.setters(out, item, names)
    out.line()
    out.line("private:")
    out.line("explicit Writer(std::span<std::byte> bytes) noexcept : m_bytes(bytes) {}")
    out.line("std::span<std::byte> m_bytes;")
    out.close("};")
    out.close("};")


def _packet_scope(packet: PacketDefinition, base: Scope, qualified: str) -> Scope:
    enums = dict(base.enums)
    enum_values = dict(base.enum_values)
    for enum in packet.enums:
        enums[enum.name] = f"{qualified}::{cpp_name(enum.name)}"
        enum_values[enum.name] = {value.name: value.value for value in enum.values}
    structures = dict(base.structures)
    structure_names = dict(base.structure_names)
    for structure in packet.structures:
        structures[structure.name] = structure
        structure_names[structure.name] = f"{qualified}::{cpp_name(structure.name)}"
    return Scope(enums, structures, structure_names, enum_values)


def _write_packet(out: CodeWriter, packet: PacketDefinition, base: Scope, namespace: str) -> None:
    name = cpp_name(packet.name)
    qualified = f"::{namespace}::{name}"
    scope = _packet_scope(packet, base, qualified)
    kind, _ = _HEADERS[packet.header]
    minimum = minimum_length(packet, scope.structures)
    names = _field_names(packet.fields, name)
    fields = FieldWriter(scope, name, end_is_open=True)
    sub_code = packet.sub_code if packet.sub_code is not None else 0
    has_sub = "true" if packet.sub_code is not None else "false"
    out.line(f"// {packet.name}: {kind} {packet.code:02X}{'' if packet.sub_code is None else f' {packet.sub_code:02X}'}"
             f", {packet.direction}.")
    out.open(f"struct {name} {{")
    out.line(f"static constexpr ::mu::protocol::PacketShape kShape{{\"{packet.name}\", "
             f"::mu::protocol::HeaderKind::{kind}, 0x{packet.code:02X}, {has_sub}, 0x{sub_code:02X}, {minimum}}};")
    out.line(f"// The length the XML declares; 0 when the length varies.")
    out.line(f"static constexpr std::size_t kDeclaredLength = {packet.length or 0};")
    for enum in packet.enums:
        out.line()
        _write_enum(out, enum)
    for structure in ordered_structures(packet.structures):
        out.line()
        _write_structure(out, structure, scope)
    out.line()
    out.open("class View {")
    out.line("public:")
    out.open("[[nodiscard]] static ::nexum::core::Result<View> Read(std::span<const std::byte> message) {")
    out.line("if (auto checked = ::mu::protocol::CheckPacket(message, kShape); !checked) {")
    out.line("    return ::tl::unexpected(checked.error());")
    out.line("}")
    out.line("return View(message);")
    out.close()
    out.line("[[nodiscard]] std::span<const std::byte> Bytes() const noexcept { return m_bytes; }")
    for item in packet.fields:
        fields.getters(out, item, names)
    out.line()
    out.line("private:")
    out.line("explicit View(std::span<const std::byte> bytes) noexcept : m_bytes(bytes) {}")
    out.line("std::span<const std::byte> m_bytes;")
    out.close("};")
    out.line()
    out.open("class Writer {")
    out.line("public:")
    out.open("[[nodiscard]] static ::nexum::core::Result<Writer> Create(std::span<std::byte> buffer) {")
    out.line("if (auto started = ::mu::protocol::StartPacket(buffer, kShape); !started) {")
    out.line("    return ::tl::unexpected(started.error());")
    out.line("}")
    out.line("Writer writer(buffer);")
    _write_defaults(out, packet.fields, names, scope)
    out.line("return writer;")
    out.close()
    out.line("[[nodiscard]] std::span<std::byte> Bytes() const noexcept { return m_bytes; }")
    for item in packet.fields:
        fields.setters(out, item, names)
    out.line()
    out.line("private:")
    out.line("explicit Writer(std::span<std::byte> bytes) noexcept : m_bytes(bytes) {}")
    out.line("std::span<std::byte> m_bytes;")
    out.close("};")
    out.close("};")


def file_scope(definitions: DefinitionFile, namespace: str, common: Scope | None) -> Scope:
    scope = Scope({}, {}, {}, {}) if common is None else Scope(dict(common.enums), dict(common.structures),
                                                               dict(common.structure_names), dict(common.enum_values))
    for enum in definitions.enums:
        scope.enums[enum.name] = f"::{namespace}::{cpp_name(enum.name)}"
        scope.enum_values[enum.name] = {value.name: value.value for value in enum.values}
    for structure in definitions.structures:
        scope.structures[structure.name] = structure
        scope.structure_names[structure.name] = f"::{namespace}::{cpp_name(structure.name)}"
    return scope


def _check_unique_names(definitions: DefinitionFile) -> None:
    """The enums, structures and packets of a file share one namespace, so two of them can never have one name. Each
    packet is also a file, and some systems do not tell upper from lower case in file names."""
    seen: set[str] = set()
    for name in [*(enum.name for enum in definitions.enums), *(item.name for item in definitions.structures),
                 *(packet.name for packet in definitions.packets)]:
        if cpp_name(name) in seen:
            raise DefinitionError(f"two definitions are named {cpp_name(name)}")
        seen.add(cpp_name(name))
    files: set[str] = {_TYPES.lower()}
    for packet in definitions.packets:
        file_name = cpp_name(packet.name).lower()
        if file_name in files:
            raise DefinitionError(f"the packet {packet.name} would have the file of another definition")
        files.add(file_name)


def with_peer_values(scope: Scope | None, peer: DefinitionFile) -> Scope:
    """Adds the enum values of another file, named as the XML names them: "ClientToServer.PetType"."""
    result = Scope({}, {}, {}, {}) if scope is None else scope
    prefix = peer.source.removesuffix(".xml").removesuffix("Packets")
    for enum in peer.enums:
        result.enum_values[f"{prefix}.{enum.name}"] = {value.name: value.value for value in enum.values}
    return result


_PREAMBLE = """\
// Generated by Tools/PacketGen from {source}, a packet definition file of OpenMU
// (Copyright 2017 MUnique, MIT license, see third_party/openmu/LICENSE). Do not edit: change the generator.
#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>

#include "Mu/Protocol/PacketFields.hpp"
"""


@dataclass
class GeneratedFile:
    """A file the generator writes: its path below the output folder, which for a header is also its include."""
    path: str
    text: str


def types_include(include_dir: str) -> str:
    return f"{include_dir}/{_TYPES}.hpp"


def packet_include(include_dir: str, packet: PacketDefinition) -> str:
    return f"{include_dir}/{cpp_name(packet.name)}.hpp"


def _start_header(source: str, includes: list[str], namespace: str) -> CodeWriter:
    out = CodeWriter()
    out.lines.extend(_PREAMBLE.format(source=source).rstrip("\n").split("\n"))
    for include in includes:
        out.line(f"#include \"{include}\"")
    out.line()
    out.line("// NOLINTBEGIN(readability-function-size)")
    out.open(f"namespace {namespace} {{")
    out.depth = 0
    return out


def _end_header(out: CodeWriter, namespace: str) -> str:
    out.line()
    out.line(f"}} // namespace {namespace}")
    out.line("// NOLINTEND(readability-function-size)")
    return out.text()


def generate_headers(definitions: DefinitionFile, namespace: str, common: Scope | None, common_include: str | None,
                     include_dir: str) -> list[GeneratedFile]:
    """Types.hpp, with the enums and structures that the packets of the file share, then one header for each packet.
    A packet uses only its own types, those of its file and the common ones, so its header includes only Types.hpp."""
    _check_unique_names(definitions)
    scope = file_scope(definitions, namespace, common)
    types = _start_header(definitions.source, [common_include] if common_include else [], namespace)
    for enum in definitions.enums:
        types.line()
        _write_enum(types, enum)
    for structure in ordered_structures(definitions.structures):
        types.line()
        _write_structure(types, structure, scope)
    files = [GeneratedFile(types_include(include_dir), _end_header(types, namespace))]
    for packet in definitions.packets:
        out = _start_header(definitions.source, [types_include(include_dir)], namespace)
        out.line()
        _write_packet(out, packet, scope, namespace)
        files.append(GeneratedFile(packet_include(include_dir, packet), _end_header(out, namespace)))
    return files


_TEST_PREAMBLE = """\
// Generated by Tools/PacketGen from {source} (OpenMU, MIT, see third_party/openmu/LICENSE). Do not edit.
// For each packet: the Writer starts it with its fixed bytes and length, the View reads it back, and the View refuses
// the same packet one byte shorter (specs/0001-rede-ate-o-mundo/spec.md, CA-03). Part {part} of {parts}.
#include <cstddef>
#include <cstdint>
#include <vector>

#include <doctest/doctest.h>

{includes}

namespace {{

template <typename Packet>
void CheckPacketShape() {{
    constexpr auto shape = Packet::kShape;
    std::vector<std::byte> bytes(shape.minimumLength);
    const auto writer = Packet::Writer::Create(bytes);
    REQUIRE(writer.has_value());
    const std::size_t codeOffset = ::mu::protocol::HeaderSize(shape.header);
    CHECK(bytes[0] == std::byte{{static_cast<std::uint8_t>(shape.header)}});
    CHECK(::mu::protocol::DeclaredLength(bytes) == shape.minimumLength);
    CHECK(bytes[codeOffset] == std::byte{{shape.code}});
    if (shape.hasSubCode) {{
        CHECK(bytes[codeOffset + 1] == std::byte{{shape.subCode}});
    }}
    CHECK(Packet::View::Read(bytes).has_value());

    std::vector<std::byte> shorter(bytes.begin(), bytes.end() - 1);
    if (::mu::protocol::HasShortLength(shape.header)) {{
        shorter[1] = std::byte{{static_cast<std::uint8_t>(shorter.size())}};
    }} else {{
        shorter[1] = std::byte{{static_cast<std::uint8_t>(shorter.size() >> 8U)}};
        shorter[2] = std::byte{{static_cast<std::uint8_t>(shorter.size())}};
    }}
    CHECK_FALSE(Packet::View::Read(shorter).has_value());
}}

}} // namespace
"""


def generate_tests(definitions: DefinitionFile, namespace: str, include_dir: str,
                   tests_prefix: str) -> list[GeneratedFile]:
    """The tests of every packet, in files of at most _PACKETS_PER_TEST_FILE packets, named tests_prefix01.cpp,
    tests_prefix02.cpp... Each file includes only the headers of its packets."""
    packets = definitions.packets
    parts = (len(packets) + _PACKETS_PER_TEST_FILE - 1) // _PACKETS_PER_TEST_FILE
    files: list[GeneratedFile] = []
    for part in range(parts):
        chunk = packets[part * _PACKETS_PER_TEST_FILE:(part + 1) * _PACKETS_PER_TEST_FILE]
        includes = "\n".join(f"#include \"{packet_include(include_dir, packet)}\"" for packet in chunk)
        out = CodeWriter()
        out.lines.extend(_TEST_PREAMBLE.format(source=definitions.source, part=part + 1, parts=parts,
                                               includes=includes).rstrip("\n").split("\n"))
        for packet in chunk:
            out.line()
            out.open(f"TEST_CASE(\"[0001-CA-03] the generated {definitions.source} packet {packet.name} keeps its "
                     f"fixed bytes and refuses a shorter packet\") {{")
            out.line(f"CheckPacketShape<::{namespace}::{cpp_name(packet.name)}>();")
            out.close()
        files.append(GeneratedFile(f"{tests_prefix}{part + 1:02d}.cpp", out.text()))
    return files


def _write_if_changed(path: Path, text: str) -> None:
    """Keeps the date of an unchanged file, so the build does not compile it again."""
    if path.is_file() and path.read_text(encoding="utf-8") == text:
        return
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_bytes(text.encode("utf-8"))


def main(arguments: list[str]) -> int:
    parser = argparse.ArgumentParser(description="Generates the C++ code of the OpenMU packet definitions.")
    parser.add_argument("--xml", required=True, type=Path)
    parser.add_argument("--namespace", required=True)
    parser.add_argument("--output-dir", required=True, type=Path, help="the folder that holds every generated file")
    parser.add_argument("--include-dir", required=True,
                        help="the folder of the headers below the output folder, as the code includes them")
    parser.add_argument("--tests-prefix", help="the path of the test files below the output folder, before the number")
    parser.add_argument("--common", type=Path, help="a file whose enums and structures every packet may use")
    parser.add_argument("--common-namespace")
    parser.add_argument("--common-include")
    parser.add_argument("--peer", type=Path, action="append", default=[],
                        help="another file whose enum values a default value may name, as in 'File.Enum.Value'")
    parser.add_argument("--list", action="store_true", help="print the files it would write, and write nothing")
    options = parser.parse_args(arguments)
    try:
        common_scope = None
        if options.common is not None:
            common = read_definitions(options.common)
            common_scope = file_scope(common, options.common_namespace, None)
        for peer in options.peer:
            common_scope = with_peer_values(common_scope, read_definitions(peer))
        definitions = read_definitions(options.xml)
        headers = generate_headers(definitions, options.namespace, common_scope, options.common_include,
                                   options.include_dir)
        tests = [] if options.tests_prefix is None else generate_tests(
            definitions, options.namespace, options.include_dir, options.tests_prefix)
        if options.list:
            for kind, files in (("header", headers), ("tests", tests)):
                for item in files:
                    print(f"{kind} {(options.output_dir / item.path).as_posix()}")
            return 0
        for item in [*headers, *tests]:
            _write_if_changed(options.output_dir / item.path, item.text)
    except (DefinitionError, ElementTree.ParseError, OSError, KeyError, ValueError) as error:
        print(f"packetgen: {options.xml}: {error}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))

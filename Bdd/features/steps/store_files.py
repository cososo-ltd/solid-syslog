"""Damaging the block store's files between runs, for the steps that corrupt a stored record."""

# A record as the store lays it out: a two-byte magic, a little-endian length,
# the message, the security policy's trailer, and one flag byte that stays 0xFF
# until the record is sent or discarded. The hosted targets default to the
# CRC-16 policy, whose trailer is two bytes.
_MAGIC = b"\xa5\x5a"
_HEADER_SIZE = 4
_CRC16_TRAILER_SIZE = 2
_FLAG_SIZE = 1
_FLAG_UNSENT = 0xFF


def first_unsent_record_body_offset(block):
    """Where the message of the first record still to send starts, or None when every record is finished with."""
    offset = 0
    while offset + _HEADER_SIZE <= len(block):
        if block[offset : offset + 2] != _MAGIC:
            raise ValueError(f"no record starts at offset {offset}")
        length = int.from_bytes(block[offset + 2 : offset + _HEADER_SIZE], "little")
        flag_offset = offset + _HEADER_SIZE + length + _CRC16_TRAILER_SIZE
        if flag_offset >= len(block):
            raise ValueError(f"the record at offset {offset} runs past the end of the block")
        if block[flag_offset] == _FLAG_UNSENT:
            return offset + _HEADER_SIZE
        offset = flag_offset + _FLAG_SIZE
    return None


def corrupt_first_unsent_record(path):
    """Flip every bit of one message byte of the first record still to send, so it fails verification."""
    with open(path, "rb") as block_file:
        block = bytearray(block_file.read())
    offset = first_unsent_record_body_offset(block)
    assert offset is not None, f"{path} holds no record still to send"
    block[offset] ^= 0xFF
    with open(path, "wb") as block_file:
        block_file.write(block)

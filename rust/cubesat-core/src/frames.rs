//! CCSDS TM Transfer Frame primary header (6 bytes, version 1).
//!
//! ```text
//!  bit  0..2   version (=0)
//!  bit  3      spacecraft id flag (0)
//!  bit  4      secondary header flag
//!  bit  5..15  APID
//!  bit 16..17  sequence flags (11 = unpaged)
//!  bit 18..31  sequence count
//!  bit 32..47  frame length - 1
//! ```
//!
//! Pure functions, no allocation: the buffer is supplied by the caller.

pub const PRIMARY_HEADER_LEN: usize = 6;
pub const SEQ_UNPAGED: u16 = 0b11;

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct PrimaryHeader {
    pub apid: u16,
    pub sequence: u16,
    /// Payload bytes *after* the header.
    pub payload_len: u16,
}

/// Serialise a header into `buf[0..6]`.
pub fn pack_primary_header(header: PrimaryHeader, buf: &mut [u8; PRIMARY_HEADER_LEN]) {
    let apid = header.apid & 0x07FF;
    let seq = header.sequence & 0x3FFF;
    let len_field = header.payload_len.saturating_sub(1);

    buf[0] = (apid >> 8) as u8; // version 0, type 0, sec-hdr 0
    buf[1] = (apid & 0xFF) as u8;
    buf[2] = ((SEQ_UNPAGED << 6) | (seq >> 8)) as u8;
    buf[3] = (seq & 0xFF) as u8;
    buf[4] = (len_field >> 8) as u8;
    buf[5] = (len_field & 0xFF) as u8;
}

/// Parse a primary header, rejecting impossible length fields.
pub fn parse_primary_header(buf: &[u8]) -> Option<PrimaryHeader> {
    if buf.len() < PRIMARY_HEADER_LEN {
        return None;
    }
    let apid = (((buf[0] & 0x07) as u16) << 8) | buf[1] as u16;
    let sequence = (((buf[2] & 0x3F) as u16) << 8) | buf[3] as u16;
    let payload_len = (((buf[4] as u16) << 8) | buf[5] as u16).checked_add(1)?;
    Some(PrimaryHeader {
        apid,
        sequence,
        payload_len,
    })
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn round_trips() {
        let h = PrimaryHeader {
            apid: 0x123,
            sequence: 0x2A5,
            payload_len: 1024,
        };
        let mut buf = [0u8; PRIMARY_HEADER_LEN];
        pack_primary_header(h, &mut buf);
        assert_eq!(parse_primary_header(&buf), Some(h));
    }

    #[test]
    fn max_fields_do_not_bleed() {
        let h = PrimaryHeader {
            apid: 0xFFFF,
            sequence: 0xFFFF,
            payload_len: 1,
        };
        let mut buf = [0u8; PRIMARY_HEADER_LEN];
        pack_primary_header(h, &mut buf);
        // Only 11 APID bits and 14 sequence bits survive.
        assert_eq!(parse_primary_header(&buf).unwrap().apid, 0x7FF);
        assert_eq!(parse_primary_header(&buf).unwrap().sequence, 0x3FFF);
    }

    #[test]
    fn rejects_short_buffers() {
        assert_eq!(parse_primary_header(&[0, 1, 2]), None);
    }

    #[test]
    fn one_byte_payload_encodes_zero() {
        let h = PrimaryHeader {
            apid: 1,
            sequence: 0,
            payload_len: 1,
        };
        let mut buf = [0u8; PRIMARY_HEADER_LEN];
        pack_primary_header(h, &mut buf);
        assert_eq!(&buf[4..6], &[0x00, 0x00]);
        assert_eq!(parse_primary_header(&buf).unwrap().payload_len, 1);
    }
}

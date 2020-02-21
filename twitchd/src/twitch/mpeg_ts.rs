#[derive(Debug, Clone, serde::Serialize, serde::Deserialize)]
pub struct SegmentMetadata {
    broadc_r: Option<u32>,
    broadc_s: u32,
    cmd: String,
    ingest_r: Option<u64>,
    ingest_s: u64,
    stream_offset: f64,
    transc_r: u64,
    transc_s: u64,
    segment_loudness: Option<f32>,
    stream_loudness: Option<f32>,
}

const MPEG_TS_SECTION_LENGTH: usize = 188;
// The metadata packet seems to always be the 3rd one
const FIRST_PES_METADATA_OFFSET: usize = MPEG_TS_SECTION_LENGTH * 3;
const SECOND_PES_METADATA_OFFSET: usize = FIRST_PES_METADATA_OFFSET + MPEG_TS_SECTION_LENGTH;

pub fn extract_metadata(data: &[u8]) -> Option<SegmentMetadata> {
    let first_pes_slice = &data[FIRST_PES_METADATA_OFFSET..SECOND_PES_METADATA_OFFSET];

    let json_start_offset = first_pes_slice.iter()
        .position(|&c| c == b'{')?;
    let unbounded_json_slice = &first_pes_slice[json_start_offset..];

    if let Some(json_end_offset) = unbounded_json_slice.iter()
        .position(|&c| c == b'}')
    {
        let json_slice = &unbounded_json_slice[..=json_end_offset];
        serde_json::from_slice(json_slice).ok()
    } else {
        // The JSON data does not fit in a single PES slice
        // Twitch uses a second one and seems to align it to the end
        let json_initial_slice = unbounded_json_slice;
        let second_pes_slice = &data[
            SECOND_PES_METADATA_OFFSET
            ..
            SECOND_PES_METADATA_OFFSET + MPEG_TS_SECTION_LENGTH
        ];
        let json_continuity_end_offset = second_pes_slice.iter()
            .position(|&c| c == b'}')?;
        let json_continuity_start_offset = (0..json_continuity_end_offset)
            .rev()
            .find(|&idx| second_pes_slice[idx] == 0xFF)? + 1;

        let json_continuity_slice = &second_pes_slice[
            json_continuity_start_offset
            ..=
            json_continuity_end_offset
        ];


        let json_slice = [json_initial_slice, json_continuity_slice].concat();
        serde_json::from_slice(&json_slice).ok()
    }
}


#[cfg(test)]
mod tests {
    macro_rules! ts_file_path {
        ($name:ident) => { concat!("../../test_samples/mpeg_ts/", stringify!($name), ".ts") }
    }

    macro_rules! extract_metadata_tests {
        ($($name:ident),* $(,)?) => {
            $(
                #[test]
                fn $name() {
                    let mpeg_data = include_bytes!(ts_file_path!($name));
                    let meta_data = super::extract_metadata(mpeg_data);
                    assert!(meta_data.is_some());
                }
            )*
        };
    }

    extract_metadata_tests! {
        source_1080p60,
        encoded_720p60,
        encoded_480p,
        encoded_160p,
        source_faulty_22_may_2019,
        source_faulty_21_feb_2020,
    }
}

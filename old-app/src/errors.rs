extern crate hyper;
extern crate native_tls;
extern crate serde_json;

use std::io;
use std::fmt;
use std::error;

#[derive(Debug)]
pub enum PlayerError {
    PollFail,
    TlsError(native_tls::Error),
    HyperError(hyper::Error),
    IoError(io::Error),
    FetchPlaylistsInfoFail(hyper::StatusCode),
    FetchPlaylistFail(hyper::StatusCode),
    FetchSegmentFail(hyper::StatusCode),
    BadPlaylistsInfoFormat(serde_json::Error),
    MalformedUrl(hyper::error::UriError),
    NoStdinAccess,
}

impl error::Error for PlayerError {
    fn description(&self) -> &str { "Player Error" }
}

impl fmt::Display for PlayerError {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        write!(f, "{:?}", self)
    }
}

impl From<hyper::Error> for PlayerError {
    fn from(error: hyper::Error) -> Self {
        PlayerError::HyperError(error)
    }
}

impl From<io::Error> for PlayerError {
    fn from(error: io::Error) -> Self {
        PlayerError::IoError(error)
    }
}

impl From<native_tls::Error> for PlayerError {
    fn from(error: native_tls::Error) -> Self {
        PlayerError::TlsError(error)
    }
}

pub fn is_fatal(error: &PlayerError) -> bool {
    use self::PlayerError::*;

    match *error {
        // Only considering Hyper's "Incomplete" errors as non fatal for now
        HyperError(hyper::error::Error::Incomplete) => false,
        _ => true
    }
}

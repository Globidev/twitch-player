extern crate hyper;
extern crate serde_json;
extern crate url;

use prelude::http::*;
use prelude::futures::*;

use self::hyper::{Request, Get};

use super::types::{AccessToken, StreamIndex, Playlist};

type ParseResult<T> = Result<T, ApiError>;

pub fn access_token(client: &HttpsClient, channel: &str, client_id: &str)
    -> impl Future<Item = AccessToken, Error = ApiError>
{
    let mut request = Request::new(Get, access_token_uri(channel));
    request.headers_mut()
        .set_raw("Client-ID", client_id);

    api_call(client, request, parse_token)
}

pub fn stream_index(client: &HttpsClient, channel: &str, token: &AccessToken)
    -> impl Future<Item = StreamIndex, Error = ApiError>
{
    let request = Request::new(Get, stream_index_uri(channel, token));

    api_call(client, request, parse_index)
}

pub fn playlist(client: &HttpsClient, url: &str)
    -> impl Future<Item = Playlist, Error = ApiError>
{
    let request = Request::new(Get, url.parse().unwrap_or_default());

    api_call(client, request, parse_playlist)
}

fn api_call<F, T>(client: &HttpsClient, request: hyper::Request, parse: F)
    -> impl Future<Item = T, Error = ApiError>
where
    F: FnOnce(hyper::Chunk) -> ParseResult<T>
{
    fetch(client, request)
        .map_err(ApiError::HttpError)
        .and_then(parse)
}

fn access_token_uri(channel: &str) -> hyper::Uri {
    let url = format!(
        "http://api.twitch.tv/api/channels/{}/access_token",
        channel.to_lowercase()
    );
    url.parse().unwrap()
}

fn stream_index_uri(channel: &str, token: &AccessToken) -> hyper::Uri {
    use self::url::form_urlencoded;

    let query_string = form_urlencoded::Serializer::new(String::new())
        .append_pair("token",        &token.token)
        .append_pair("sig",          &token.signature)
        .append_pair("allow_source", "true")
        .append_pair("type",         "any")
        .finish();

    let url = format!(
        "http://usher.twitch.tv/api/channel/hls/{}.m3u8?{}",
        channel.to_lowercase(),
        query_string
    );

    url.parse().unwrap()
}

fn parse_token (chunk: hyper::Chunk) -> ParseResult<AccessToken> {
    use self::serde_json::from_slice as json_decode;

    json_decode(&chunk)
        .map_err(|error| ApiError::FormatError(error.to_string()))
}


fn parse_index (chunk: hyper::Chunk) -> ParseResult<StreamIndex> {
    use super::m3u8::{ParseError, parse_stream_index};

    parse_stream_index(&chunk)
        .map_err(|ParseError(error)| ApiError::FormatError(error))
}

fn parse_playlist(data: hyper::Chunk) -> ParseResult<Playlist> {
    use std::str::from_utf8;
    use std::string::ToString;

    from_utf8(&data)
        .map_err(|error| ApiError::FormatError(error.to_string()))
        .map(|string| {
            string.split('\n')
                .filter(|line| line.starts_with("index-") || line.starts_with("http://"))
                .map(ToString::to_string)
                .collect()
        })
}

#[derive(Debug)]
pub enum ApiError {
    HttpError(HttpError),
    FormatError(String)
}

use std::{error, fmt};

impl error::Error for ApiError {
    fn description(&self) -> &str { "Api Error" }
}

impl fmt::Display for ApiError {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        write!(f, "{:?}", self)
    }
}

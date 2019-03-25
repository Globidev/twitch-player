use crate::prelude::http::*;
use crate::prelude::futures::*;

use super::types::{AccessToken, StreamIndex, Playlist};

use url::form_urlencoded;

type ParseResult<T> = Result<T, ApiError>;

pub fn access_token(client: &HttpsClient, channel: &str, client_id: &str, oauth: Option<&str>)
    -> impl Future<Item = AccessToken, Error = ApiError>
{
    let request = hyper::Request::get(access_token_url(channel, oauth))
        .header("Client-ID", client_id)
        .body(hyper::Body::default())
        .expect("Request Building error: Access Token");

    api_call(client, request, parse_token)
}

pub fn stream_index(client: &HttpsClient, channel: &str, token: &AccessToken)
    -> impl Future<Item = StreamIndex, Error = ApiError>
{
    let request = hyper::Request::get(stream_index_url(channel, token))
        .body(hyper::Body::default())
        .expect("Request Building error: Stream Index");

    api_call(client, request, parse_index)
}

pub fn playlist(client: &HttpsClient, url: &str)
    -> impl Future<Item = Playlist, Error = ApiError>
{
    let request = hyper::Request::get(url)
        .body(hyper::Body::default())
        .expect("Request Building error: Playlist");

    api_call(client, request, parse_playlist)
}

fn api_call<F, T>(client: &HttpsClient, request: Request, parse: F)
    -> impl Future<Item = T, Error = ApiError>
where
    F: FnOnce(hyper::Chunk) -> ParseResult<T>
{
    fetch(client, request)
        .map_err(ApiError::HttpError)
        .and_then(parse)
}

fn access_token_url(channel: &str, oauth: Option<&str>) -> String {
    let query_string = match form_urlencoded::Serializer::new(String::new()) {
        mut query => {
            query
                .append_pair("platform", "_")
                .append_pair("player_backend", "mediaplayer")
                .append_pair("player_type", "site");

            if let Some(token) = oauth {
                query.append_pair("oauth_token", token);
            }

            query.finish()
        }
    };

    format!(
        "https://api.twitch.tv/api/channels/{}/access_token?{}",
        channel.to_lowercase(),
        query_string
    )
}

fn stream_index_url(channel: &str, token: &AccessToken) -> String {
    let query_string = form_urlencoded::Serializer::new(String::new())
        .append_pair("token",                      &token.token)
        .append_pair("sig",                        &token.signature)
        .append_pair("allow_source",               "true")
        .append_pair("fast_bread",                 "true")
        .append_pair("baking_bread",               "true")
        .append_pair("baking_brownies",            "true")
        .append_pair("baking_brownies_timeout",    "1050")
        .append_pair("playlist_include_framerate", "true")
        // .append_pair("reassignments_supported",    "true")
        .append_pair("cdm",                        "wv")
        .append_pair("player_backend",             "mediaplayer")
        .append_pair("rtqos",                      "control")
        .append_pair("preferred_codecs",           "avc1")
        // TODO: figure out that parameter's meaning
        // .append_pair("p",                          "7036871")
        .finish();

    format!(
        "http://usher.twitch.tv/api/channel/hls/{}.m3u8?{}",
        channel.to_lowercase(),
        query_string
    )
}

fn parse_token(chunk: hyper::Chunk) -> ParseResult<AccessToken> {
    use serde_json::from_slice as json_decode;

    json_decode(&chunk)
        .map_err(|error| ApiError::FormatError(error.to_string()))
}


fn parse_index(chunk: hyper::Chunk) -> ParseResult<StreamIndex> {
    use super::m3u8::{ParseError, parse_stream_index};

    parse_stream_index(&chunk)
        .map_err(|ParseError(error)| ApiError::FormatError(error))
}

fn parse_playlist(chunk: hyper::Chunk) -> ParseResult<Playlist> {
    use super::m3u8::{ParseError, parse_playlist};

    parse_playlist(&chunk)
        .map_err(|ParseError(error)| ApiError::FormatError(error))
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

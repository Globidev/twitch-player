use super::types::{AccessToken, Playlist, StreamIndex};
use crate::prelude::http::*;

use url::form_urlencoded;

type ApiResult<T> = Result<T, ApiError>;

pub fn access_token(
    client: &HttpsClient,
    channel: &str,
    client_id: &str,
    oauth: Option<&str>,
) -> impl Future<Output = ApiResult<AccessToken>> {
    let request = hyper::Request::get(access_token_url(channel, oauth))
        .header("Client-ID", client_id)
        .body(hyper::Body::default())
        .expect("Request Building error: Access Token");

    api_call(client, request, parse_token)
}

pub fn stream_index(
    client: &HttpsClient,
    channel: &str,
    token: &AccessToken,
) -> impl Future<Output = ApiResult<StreamIndex>> {
    let request = hyper::Request::get(stream_index_url(channel, token))
        .body(hyper::Body::default())
        .expect("Request Building error: Stream Index");

    api_call(client, request, parse_index)
}

use futures::prelude::*;

pub fn playlist(client: &HttpsClient, url: &str) -> impl Future<Output = ApiResult<Playlist>> {
    let request = hyper::Request::get(url)
        .body(hyper::Body::default())
        .expect("Request Building error: Playlist");

    api_call(client, request, parse_playlist)
}

fn api_call<F, T>(
    client: &HttpsClient,
    request: Request,
    parse: F,
) -> impl Future<Output = ApiResult<T>>
where
    F: FnOnce(&[u8]) -> ApiResult<T>,
{
    fetch(client, request)
        .map_err(ApiError::HttpError)
        .map(|d| parse(&d?))
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
        .append_pair("token", &token.token)
        .append_pair("sig", &token.signature)
        .append_pair("allow_source", "true")
        .append_pair("fast_bread", "true")
        .append_pair("baking_bread", "true")
        .append_pair("baking_brownies", "true")
        .append_pair("baking_brownies_timeout", "1050")
        .append_pair("playlist_include_framerate", "true")
        // .append_pair("reassignments_supported",    "true")
        .append_pair("cdm", "wv")
        .append_pair("player_backend", "mediaplayer")
        .append_pair("rtqos", "control")
        .append_pair("preferred_codecs", "avc1")
        // TODO: figure out that parameter's meaning
        // .append_pair("p",                          "7036871")
        .finish();

    format!(
        "http://usher.twitch.tv/api/channel/hls/{}.m3u8?{}",
        channel.to_lowercase(),
        query_string
    )
}

fn parse_token(data: &[u8]) -> ApiResult<AccessToken> {
    use serde_json::from_slice as json_decode;

    json_decode(data).map_err(|error| ApiError::FormatError(error.to_string()))
}

fn parse_index(data: &[u8]) -> ApiResult<StreamIndex> {
    use super::m3u8::{parse_stream_index, ParseError};

    parse_stream_index(data).map_err(|ParseError(error)| ApiError::FormatError(error))
}

fn parse_playlist(data: &[u8]) -> ApiResult<Playlist> {
    use super::m3u8::{parse_playlist, ParseError};

    parse_playlist(data).map_err(|ParseError(error)| ApiError::FormatError(error))
}

#[derive(Debug)]
pub enum ApiError {
    HttpError(HttpError),
    FormatError(String),
}

use std::{error, fmt};

impl error::Error for ApiError {
    fn description(&self) -> &str {
        "Api Error"
    }
}

impl fmt::Display for ApiError {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        write!(f, "{:?}", self)
    }
}

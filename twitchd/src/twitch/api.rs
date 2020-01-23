use super::{
    m3u8::{self, ParseError},
    types::{AccessToken, Playlist, StreamIndex},
};
use crate::prelude::http::*;

use futures::prelude::*;
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

    api_call(client, request, |d| serde_json::from_slice(d))
}

pub fn stream_index(
    client: &HttpsClient,
    channel: &str,
    token: &AccessToken,
) -> impl Future<Output = ApiResult<StreamIndex>> {
    let request = hyper::Request::get(stream_index_url(channel, token))
        .body(hyper::Body::default())
        .expect("Request Building error: Stream Index");

    api_call(client, request, m3u8::parse_stream_index)
}

pub fn playlist(client: &HttpsClient, url: &str) -> impl Future<Output = ApiResult<Playlist>> {
    let request = hyper::Request::get(url)
        .body(hyper::Body::default())
        .expect("Request Building error: Playlist");

    api_call(client, request, m3u8::parse_playlist)
}

fn api_call<F, T, E>(
    client: &HttpsClient,
    request: Request,
    parse: F,
) -> impl Future<Output = ApiResult<T>>
where
    F: FnOnce(&[u8]) -> Result<T, E>,
    ApiError: From<E>,
{
    fetch(client, request)
        .map_err(ApiError::Http)
        .map(|data| Ok(parse(&data?)?))
}

fn access_token_url(channel: &str, oauth: Option<&str>) -> String {
    let query_string = {
        let mut query = form_urlencoded::Serializer::new(String::new());

        query
            .append_pair("platform", "_")
            .append_pair("player_backend", "mediaplayer")
            .append_pair("player_type", "site");

        if let Some(token) = oauth {
            query.append_pair("oauth_token", token);
        }

        query.finish()
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
        .append_pair("cdm", "wv")
        .append_pair("player_backend", "mediaplayer")
        .append_pair("rtqos", "control")
        .append_pair("preferred_codecs", "avc1")
        .finish();

    format!(
        "http://usher.twitch.tv/api/channel/hls/{}.m3u8?{}",
        channel.to_lowercase(),
        query_string
    )
}

#[derive(thiserror::Error, Debug)]
pub enum ApiError {
    #[error("HTTP error: {0}")]
    Http(HttpError),
    #[error("Failed to parse M3U8: {0}")]
    M3U8(#[from] ParseError),
    #[error("Failed to decode access token: {0}")]
    AccessToken(#[from] serde_json::Error),
}

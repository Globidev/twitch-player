extern crate futures;
extern crate hyper;
extern crate num_cpus;
extern crate serde_json;
extern crate tokio_core;
extern crate url;

#[derive(Debug, Deserialize)]
pub struct AccessToken {
    token: String,
    #[serde(rename = "sig")]
    signature: String
}

use self::futures::{Future, Stream};
use self::hyper::{Client, client::HttpConnector};

type HttpClient = Client<::hyper_tls::HttpsConnector<HttpConnector>>;

fn access_token_uri(channel: &str) -> hyper::Uri {
    let url = format!(
        "http://api.twitch.tv/api/channels/{}/access_token",
        channel.to_lowercase()
    );
    url.parse().unwrap()
}

pub fn access_token(client: &HttpClient, channel: &str, client_id: &str)
    -> impl Future<Item = AccessToken, Error = hyper::Error>
{
    use self::hyper::{Request, Get};
    use self::serde_json::from_slice as json_decode;

    let mut request = Request::new(Get, access_token_uri(channel));
    request.headers_mut()
        .set_raw("Client-ID", client_id);

    client.request(request)
        .and_then(|response| {
            println!("{:?}", response);
            response.body().concat2()
        })
        .map(|chunk| json_decode(&chunk).unwrap())
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

#[derive(Debug)]
pub struct PlaylistInfo {
    url: String
}

#[derive(Debug)]
pub struct StreamIndex {
    playlist_infos: Vec<PlaylistInfo>
}

fn parse_stream_index(data: &[u8]) -> StreamIndex {
    use std::str::from_utf8;

    named!(playlist_info_parser<PlaylistInfo>, do_parse!(
        take_until!("http://") >>
        url: take_until!("\n") >> tag!("\n") >>
        (PlaylistInfo { url: String::from(from_utf8(url).unwrap_or_default()) })
    ));

    named!(stream_index_parser<StreamIndex>, do_parse!(
        take_until!("#EXT-X-MEDIA:TYPE") >>
        playlist_infos: many1!(playlist_info_parser) >>
        (StreamIndex { playlist_infos: playlist_infos })
    ));

    let (_, o) = stream_index_parser(data).unwrap();
    o
}

pub fn stream_index(client: &HttpClient, channel: &str, token: &AccessToken)
    -> impl Future<Item = StreamIndex, Error = hyper::Error>
{
    client.get(stream_index_uri(channel, token))
        .and_then(|response| {
            println!("{:?}", response);
            response.body().concat2()
        })
        .map(|chunk| parse_stream_index(&chunk))
}

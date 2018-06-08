extern crate url;
extern crate http;
extern crate hyper;
extern crate hyper_tls;
extern crate num_cpus;

pub use self::http::method::Method;
pub use self::http::header;

use self::hyper::client::HttpConnector;
use self::hyper_tls::{HttpsConnector, Error as TlsError};

use super::futures::*;

use std::collections::HashMap;

pub type Request = hyper::Request<hyper::Body>;
pub type Response = hyper::Response<hyper::Body>;
pub type HttpsClient = hyper::Client<HttpsConnector<HttpConnector>>;
pub type QueryParams = HashMap<String, String>;
pub type ResponseSink = hyper::body::Sender;

pub fn http_client() -> Result<HttpsClient, TlsError> {
    let connector = HttpsConnector::new(num_cpus::get())?;

    let client = hyper::Client::builder()
        .build(connector);

    Ok(client)
}

pub fn parse_query_params(query: &str) -> QueryParams {
    use self::url::form_urlencoded::parse as parse_query;

    parse_query(query.as_bytes())
        .map(|(k, v)| (String::from(k), String::from(v)))
        .collect()
}

pub fn fetch(client: &HttpsClient, request: Request)
    -> impl Future<Item = hyper::Chunk, Error = HttpError>
{
    fetch_streamed(client, request)
        .concat2()
}

pub fn fetch_streamed(client: &HttpsClient, request: Request)
    -> impl Stream<Item = hyper::Chunk, Error = HttpError>
{
    client.request(request)
        .map_err(HttpError::NetworkError)
        .map(stream_response)
        .flatten_stream()
}

pub fn streaming_response() -> (ResponseSink, Response) {
    let (sink, body) = hyper::Body::channel();
    let response = Response::new(body);

    (sink, response)
}

fn stream_response(response: Response) -> BoxStream<hyper::Chunk, HttpError> {
    match response.status() {
        hyper::StatusCode::OK => {
            let streamed_body = response.into_body()
                .map_err(HttpError::NetworkError);
            Box::new(streamed_body)
        },
        status => {
            let error = future::err(HttpError::BadStatus(status));
            Box::new(error.into_stream())
        }
    }
}

#[derive(Debug)]
pub enum HttpError {
    NetworkError(hyper::Error),
    BadStatus(hyper::StatusCode),
}

use std::{error, fmt};

impl error::Error for HttpError {
    fn description(&self) -> &str { "Http Error" }
}

impl fmt::Display for HttpError {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        write!(f, "{:?}", self)
    }
}

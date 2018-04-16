extern crate hyper;
extern crate hyper_tls;
extern crate num_cpus;

use self::hyper::{Client, client::HttpConnector};
use self::hyper_tls::{HttpsConnector, Error as TlsError};

use super::asio::Handle;
use super::futures::*;

pub type HttpsClient = Client<HttpsConnector<HttpConnector>>;

pub fn http_client(handle: &Handle) -> Result<HttpsClient, TlsError> {
    let connector = HttpsConnector::new(num_cpus::get(), handle)?;

    let client = Client::configure()
        .connector(connector)
        .build(handle);

    Ok(client)
}

pub fn fetch(client: &HttpsClient, request: hyper::Request)
    -> impl Future<Item = hyper::Chunk, Error = HttpError>
{
    client.request(request)
        .map_err(HttpError::NetworkError)
        .and_then(read_response)
}

fn read_response(response: hyper::Response)
    -> Box<Future<Item = hyper::Chunk, Error = HttpError>>
{
    match response.status() {
        hyper::StatusCode::Ok => {
            let full_body = response.body()
                .concat2()
                .map_err(HttpError::NetworkError);
            Box::new(full_body)
        },
        status => {
            let error = future::err(HttpError::BadStatus(status));
            Box::new(error)
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

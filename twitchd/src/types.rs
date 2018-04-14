extern crate hyper;
extern crate hyper_tls;
extern crate tokio_core;

use self::hyper::{Client, client::HttpConnector};
use self::hyper_tls::HttpsConnector;

pub type HttpsClient = Client<HttpsConnector<HttpConnector>>;
pub type Handle = tokio_core::reactor::Handle;

#[macro_use]
extern crate serde_derive;
#[macro_use]
extern crate nom;

extern crate hyper;
extern crate hyper_tls;
extern crate tokio_core;

extern crate num_cpus;

extern crate futures;

mod twitch;

use futures::Future;

const CLIENT_ID: &str = "alcht5gave31yctuzv48v2i1hlwq53";

fn main() {
    use ::hyper_tls::HttpsConnector;

    let mut core = tokio_core::reactor::Core::new().unwrap();
    let handle = &core.handle();

    let client = hyper::Client::configure()
        .connector(HttpsConnector::new(num_cpus::get(), handle).unwrap())
        .build(handle);

    let channel = ::std::env::args().nth(1).unwrap_or_default();
    let app = twitch::api::access_token(&client, &channel, CLIENT_ID)
        .and_then(|token| twitch::api::stream_index(&client, &channel, &token));

    match core.run(app) {
        Ok(index)  => println!("{:#?}", index),
        Err(error) => println!("{:#?}", error),
    }
}

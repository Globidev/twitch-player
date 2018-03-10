extern crate futures;
extern crate hyper;
extern crate tokio_core;

use std::io::{self, Write};
use self::futures::{Future, Stream};
use self::hyper::{Client, Result};
use self::tokio_core::reactor::Core;

pub fn run() -> Result<()> {
    let mut core = Core::new()?;
    let client = Client::new(&core.handle());

    let uri = "http://httpbin.org/ip".parse()?;
    let work = client.get(uri).and_then(|res| {
        println!("Response: {}", res.status());

        res.body().for_each(|chunk| {
            io::stdout()
                .write_all(&chunk)
                .map(|_| ())
                .map_err(From::from)
        })
    });

    core.run(work)
}

#![feature(drain_filter)]

#[macro_use] extern crate nom;
#[macro_use] extern crate serde_derive;
#[macro_use] extern crate structopt;

mod twitch;
mod server;
mod prelude;
mod options;

fn main() {
    use options::Options;
    use structopt::StructOpt;

    let opts = Options::from_args();

    println!("Running with options: {:#?}", opts);

    server::run(opts);
}

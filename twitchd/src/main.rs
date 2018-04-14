#[macro_use] extern crate serde_derive;
#[macro_use] extern crate nom;

mod twitch;
mod server;
mod types;

fn main() {
    let addr = "0.0.0.0:8080".parse().unwrap();

    server::run(&addr);
}

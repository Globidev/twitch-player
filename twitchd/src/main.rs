#![cfg_attr(not(debug_assertions), windows_subsystem = "windows")]

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

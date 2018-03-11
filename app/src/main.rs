#![feature(conservative_impl_trait)]
#![windows_subsystem = "windows"]

#[macro_use]
extern crate serde_derive;
#[macro_use]
extern crate structopt;

mod windows;
mod player;
mod gui;
mod types;
mod errors;
mod options;
mod process;

use std::thread;
use options::Options;

fn run_app(opts: Options) -> thread::Result<()> {
    let opts_player = opts.clone();

    let (player_out, player_in) = types::player_channel();
    let (gui_out, gui_in) = types::gui_channel();

    let gui_out_clone = gui_out.clone();
    let player = thread::spawn(move || player::run(opts_player, player_in, gui_out_clone));
    let gui = thread::spawn(move || gui::run(opts, gui_in, player_out));

    player.join()?;
    gui_out.send(types::GuiMessage::CanExit).unwrap_or_default();
    gui.join()?;

    Ok(())
}

fn main() {
    use structopt::StructOpt;

    let opts = Options::from_args();

    if let Err(error) = run_app(opts) {
        eprintln!("App panicked: {:?}", error);
    }
}

#![feature(conservative_impl_trait)]
// #![windows_subsystem = "windows"]

#[macro_use]
extern crate serde_derive;

mod windows;
mod player;
mod gui;
mod types;

use std::thread;

fn run_app() -> thread::Result<()> {
    let (player_out, player_in) = types::player_channel();
    let (gui_out, gui_in) = types::gui_channel();

    let gui_out_clone = gui_out.clone();
    let player = thread::spawn(move || player::run(player_in, gui_out_clone));
    let gui = thread::spawn(move || gui::run(gui_in, player_out));

    player.join()?;
    gui_out.send(types::GuiMessage::CanExit).unwrap_or_default();
    gui.join()?;

    Ok(())
}

fn main() {
    if let Err(error) = run_app() {
        eprintln!("App panicked: {:?}", error);
    }
}

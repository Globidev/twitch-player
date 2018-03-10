use types::{PlayerReceiver, GuiSender};

pub fn run(messages_in: PlayerReceiver, messages_out: GuiSender) {
    use types::PlayerMessage::*;
    use types::GuiMessage::*;

    while let Ok(message) = messages_in.recv() {
        match message {
            AppQuit => {
                println!("Stopping...");
                break
            }
        }
    }
}

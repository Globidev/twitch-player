extern crate winres;

fn main() {
    let mut res = winres::WindowsResource::new();
    res.set_icon("build/icon.ico");
    res.set_resource_file("build/app.rc");
    res.compile().unwrap();
}

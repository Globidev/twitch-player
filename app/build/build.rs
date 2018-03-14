#[cfg(windows)]
extern crate winres;

#[cfg(windows)]
fn main() {
    let mut res = winres::WindowsResource::new();
    res.set_icon("build/icon.ico");
    res.set_resource_file("build/app.rc");
    res.compile().unwrap();
}

#[cfg(unix)]
fn main() {
    println!("cargo:rustc-link-lib=dylib=X11");
}

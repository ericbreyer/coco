use std::env;
use std::path::PathBuf;

fn main() {
    
    println!("cargo:rustc-link-search=native={}", "/Users/ebreyer/Documents/GitHub/miniCoroutines/build/");
    println!("cargo:rustc-link-lib=static={}", "coco");

    // let bindings = bindgen::builder()
    //     // The input header we would like to generate
    //     // bindings for.
    //     .header("wrapper.h").clang_arg("-F/Users/ebreyer/Documents/GitHub/miniCoroutines/src/")
    //     // Tell cargo to invalidate the built crate whenever any of the
    //     // included header files changed.
    //     .parse_callbacks(Box::new(bindgen::CargoCallbacks))
    //     // Finish the builder and generate the bindings.
    //     .generate()
    //     // Unwrap the Result and panic on failure.
    //     .expect("Unable to generate bindings");

    // // Write the bindings to the $OUT_DIR/bindings.rs file.
    // let out_path = PathBuf::from(env::var("OUT_DIR").unwrap());
    // bindings
    //     .write_to_file(out_path.join("bindings.rs"))
    //     .expect("Couldn't write bindings!");
}
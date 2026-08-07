mod bridge;

fn main() {
    //let file = std::env::args().nth(1).unwrap();
    let file = "testcode.txt";
    
    bridge::ffi::run_file(&file);

    bridge::ffi::run_file("test_operators.fling");

    bridge::ffi::run_file("while.test");

    bridge::ffi::run_file("prime.txt");

    bridge::ffi::runREPL();
}

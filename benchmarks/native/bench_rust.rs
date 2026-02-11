use std::env;
use std::time::Instant;

#[derive(Clone, Copy)]
struct BenchmarkOptions {
    iterations: usize,
    sum_n: u64,
    prime_n: usize,
    matrix_n: usize,
}

fn parse_options() -> BenchmarkOptions {
    let mut options = BenchmarkOptions {
        iterations: 5,
        sum_n: 5_000_000,
        prime_n: 30_000,
        matrix_n: 48,
    };

    let mut args = env::args().skip(1);
    while let Some(arg) = args.next() {
        match arg.as_str() {
            "--iterations" => {
                let value = args.next().expect("Missing value for --iterations.");
                options.iterations = value.parse().expect("Invalid value for --iterations.");
            }
            "--sum-n" => {
                let value = args.next().expect("Missing value for --sum-n.");
                options.sum_n = value.parse().expect("Invalid value for --sum-n.");
            }
            "--prime-n" | "--sieve-n" => {
                let value = args.next().expect("Missing value for --prime-n.");
                options.prime_n = value.parse().expect("Invalid value for --prime-n.");
            }
            "--matrix-n" => {
                let value = args.next().expect("Missing value for --matrix-n.");
                options.matrix_n = value.parse().expect("Invalid value for --matrix-n.");
            }
            _ => {
                panic!("Unknown option '{}'.", arg);
            }
        }
    }

    if options.iterations == 0 {
        panic!("--iterations must be greater than zero.");
    }

    options
}

fn run_sum_xor(n: u64) -> u64 {
    let mut acc = 0u64;
    for i in 1..=n {
        acc = acc.wrapping_add((i ^ (i >> 3)).wrapping_add(i % 8));
    }
    acc
}

fn run_prime_trial(n: usize) -> u64 {
    if n < 2 {
        return 0;
    }

    let mut prime_count = 0u64;
    let mut checksum = 0u64;
    for candidate in 2..=n {
        let mut divisor = 2usize;
        let mut is_prime = true;
        while divisor * divisor <= candidate {
            if candidate % divisor == 0 {
                is_prime = false;
                break;
            }
            divisor += 1;
        }

        if !is_prime {
            continue;
        }

        prime_count += 1;
        checksum = checksum.wrapping_add((candidate as u64).wrapping_mul((prime_count % 16) + 1));
    }

    (prime_count << 32) ^ checksum
}

fn run_affine_grid(n: usize) -> u64 {
    if n == 0 {
        return 0;
    }

    let mut checksum = 0u64;
    for row in 0..n {
        for col in 0..n {
            let mut acc = 0u64;
            for k in 0..n {
                let a = (((row * 131) + (k * 17) + 13) % 256) as u64;
                let b = (((k * 19) + (col * 97) + 53) % 256) as u64;
                acc = acc.wrapping_add(a.wrapping_mul(b));
            }

            let index = (row as u64)
                .wrapping_mul(n as u64)
                .wrapping_add(col as u64);
            checksum ^= acc.wrapping_add(index.wrapping_mul(2_654_435_761u64));
        }
    }

    checksum
}

fn mix_checksum(current: u64, value: u64, iteration: u64) -> u64 {
    let mixed = current
        ^ value
            .wrapping_add(0x9e37_79b9_7f4a_7c15)
            .wrapping_add(iteration << 6)
            .wrapping_add(iteration >> 2);
    mixed.rotate_left(13)
}

fn print_result(algorithm: &str, iterations: usize, total_ms: f64, checksum: u64) {
    let mean_ms = total_ms / iterations as f64;
    println!(
        "rust,{},{},{:.3},{:.6},{}",
        algorithm, iterations, total_ms, mean_ms, checksum
    );
}

fn main() {
    let options = parse_options();
    println!("language,algorithm,iterations,total_ms,mean_ms,checksum");

    let started = Instant::now();
    let mut sum_checksum = 0u64;
    for i in 0..options.iterations {
        sum_checksum = mix_checksum(sum_checksum, run_sum_xor(options.sum_n), i as u64);
    }
    print_result(
        "sum_xor",
        options.iterations,
        started.elapsed().as_secs_f64() * 1000.0,
        sum_checksum,
    );

    let started = Instant::now();
    let mut prime_checksum = 0u64;
    for i in 0..options.iterations {
        prime_checksum = mix_checksum(prime_checksum, run_prime_trial(options.prime_n), i as u64);
    }
    print_result(
        "prime_trial",
        options.iterations,
        started.elapsed().as_secs_f64() * 1000.0,
        prime_checksum,
    );

    let started = Instant::now();
    let mut grid_checksum = 0u64;
    for i in 0..options.iterations {
        grid_checksum = mix_checksum(grid_checksum, run_affine_grid(options.matrix_n), i as u64);
    }
    print_result(
        "affine_grid",
        options.iterations,
        started.elapsed().as_secs_f64() * 1000.0,
        grid_checksum,
    );
}

# histfactory-fitter-demo [![github CI](https://github.com/umd-lhcb/histfactory-fitter-demo/workflows/CI/badge.svg?branch=master)](https://github.com/umd-lhcb/histfactory-fitter-demo/actions?query=workflow%3ACI)

A simple HistFactory fitter based on Phoebe's demo R(D*) fitter.

For more info on Phoebe's original fitter, take a look at [this TWiki](https://twiki.cern.ch/twiki/bin/viewauth/LHCbPhysics/HistFactoryInfo).
For a more faithful presentation of Phoebe's code, take a look at version [`0.1.1`](https://github.com/umd-lhcb/histfactory-fitter-demo/tree/0.1.1).


## How to run this demo

1. Install `nix` as instructed [here](https://github.com/umd-lhcb/root-curated#install-nix-on-macos).
2. Type `nix develop`.
3. In the resulting shell prompt, type `make fit`.
    - Fit plot is stored in `gen`.
    - Full fit outputs are in `gen/fit_output`.
    - Fit log are timestamped and stored in `logs`.


## Switch ROOT version

1. Run `make clean`.

    **Note**: This will delete everything inside `gen` folder.

2. Edit `flake.nix` to pick the ROOT version you want to use:

    ```nix
      buildInputs = with pkgs; [
        clang-tools # For clang-format
        clang-format-all

        # Pick your favorite version of ROOT here
        #root_6_24_02
        root_6_16_00
        #root_5_34_38
      ];
    ```
3. Repeat the processed described in [the previous section](#how-to-run-this-demo).


## Benchmarks

Here we list the time used to fit running on various systems.

| OS | CPU model | ROOT version | Fit time [s] | Prep time [s] |
|---|---|---|---|---|
| Linux (NixOS) | `Intel(R) Core(TM) i7-7700HQ CPU @ 2.80GHz` | `6.24.02` | `53.83` | `19.93` |
| Linux (NixOS) | `Intel(R) Core(TM) i7-7700HQ CPU @ 2.80GHz` | `6.16.00` | `68.71` | `99.54` |
| Linux (NixOS) | `Intel(R) Core(TM) i7-7700HQ CPU @ 2.80GHz` | `6.12.06` | `68.30` | `99.42` |
| Linux (NixOS) | `Intel(R) Core(TM) i7-7700HQ CPU @ 2.80GHz` | `5.34.38` | `70.15` | `104.99` |
| Linux (NixOS) | `AMD Ryzen 7 PRO 4750U with Radeon Graphics` | `6.24.02` | `62.03` | `13.87` |
| Linux (NixOS) | `AMD Ryzen 7 PRO 4750U with Radeon Graphics` | `6.16.00` | `74.71` | `92.27` |
| Linux (NixOS) | `AMD Ryzen 7 PRO 4750U with Radeon Graphics` | `6.12.06` | `73.29` | `89.97` |
| Linux (NixOS) | `AMD Ryzen 7 PRO 4750U with Radeon Graphics` | `5.34.38` | `72.81` | `90.75` |
| macOS 10.15.7 (Catalina) | `Intel(R) Core(TM) i9-9880H CPU @ 2.30GHz` | `6.16.00` | `72.71` | `94.78` |
| macOS 10.15.7 (Catalina) | `Intel(R) Core(TM) i9-9880H CPU @ 2.30GHz` | `6.12.06` | `70.66` | `87.22` |
| macOS 10.15.7 (Catalina) | `Intel(R) Core(TM) i9-9880H CPU @ 2.30GHz` | `5.34.38` | `71.60` | `90.06` |
| macOS 11.4 (Big Sur) | `Intel(R) Core(TM) i7-1068NG7 CPU @ 2.30GHz` | `6.16.00` | `59.85` | `79.18` |
| macOS 11.4 (Big Sur) | `Intel(R) Core(TM) i7-1068NG7 CPU @ 2.30GHz` | `5.34.38` | `60.13` | `83.27` |
| macOS 11.2.3 (Big Sur) | `Intel(R) Core(TM) i7-9750H CPU @ 2.60GHz` | `5.34.38` | `77.52` | `105.19` |
| macOS 11.2.3 (Big Sur) | `Intel(R) Core(TM) i7-9750H CPU @ 2.60GHz` | `6.16.00` | `73.35` | `96.92` |
| macOS 11.2.3 (Big Sur) | `Intel(R) Core(TM) i7-9750H CPU @ 2.60GHz` | `6.24.02` | `62.06` | `15.94` |

### Find your CPU model name

- For Linux:

    ```shell
    cat /proc/cpuinfo | grep "model name"
    ```

- For macOS:

    ```shell
    sysctl -a | grep machdep.cpu.brand
    ```


## Coding convention

It is recommended that before you commit, run `fmtall` so that all C++ code are
formatted in a consistent way.

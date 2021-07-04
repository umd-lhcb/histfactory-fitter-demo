# histfactory-fitter-demo
Phoebe's demo R(D*) HistFactory fitter. For more info, take a look at [this TWiki](https://twiki.cern.ch/twiki/bin/viewauth/LHCbPhysics/HistFactoryInfo).


## How to run this demo

1. Install `nix` as instructed [here](https://github.com/umd-lhcb/root-curated#install-nix-on-macos).
2. Type `nix develop`.
3. In the resulting shell prompt, type `make fit`.
    - The fit outputs are stored in `gen`.
    - Some diagnostics outputs are in `results`.
    - Fit log are timestamped and stored in `logs`.


## Switch ROOT version

1. Run `make clean`.

    **Note**: This will delete everything inside `gen` and the `results` folder.

2. Edit `flake.nix` to pick the ROOT version you want to use:

    ```nix
      buildInputs = with pkgs; [
        clang-tools # For clang-format
        clang-format-all

        # Pick your favorite version of ROOT here
        #root
        root_6_12_06
        #root_5_34_38
      ];
    ```
3. Repeat the processed described in [the previous section](#how-to-run-this-demo).


## Benchmarks

Here we list the time used to fit running on various systems.

| OS | CPU model | ROOT version | Fit time [s] | Prep time [s] |
|---|---|---|---|---|
| Linux (NixOS) | `Intel(R) Core(TM) i7-7700HQ CPU @ 2.80GHz` | `6.12.06` | `68.30` | `99.42` |
| Linux (NixOS) | `Intel(R) Core(TM) i7-7700HQ CPU @ 2.80GHz` | `5.34.38` | `70.15` | `104.99` |
| Linux (NixOS) | `AMD Ryzen 7 PRO 4750U with Radeon Graphics` | `6.12.06` | `73.29` | `89.97` |
| Linux (NixOS) | `AMD Ryzen 7 PRO 4750U with Radeon Graphics` | `5.34.38` | `72.81` | `90.75` |


### Find your CPU model name

- For Linux:

    ```shell
    cat /proc/cpuinfo | grep "model name"
    ```

- For macOS:

    ```shell
    sysctl -a | grep machdep.cpu.brand
    ```

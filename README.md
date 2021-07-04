# histfactory-fitter-demo
Phoebe's demo R(D*) HistFactory fitter.


## How to run this demo

1. Install `nix` as instructed [here](https://github.com/umd-lhcb/root-curated#install-nix-on-macos).
2. Type `nix develop`.
3. In the resulting shell prompt, type `make fit`.
    - The fit outputs are stored in `gen`.
    - Some diagnostics outputs are in `results`.
    - Fit log are timestamped and stored in `logs`.


## Switch ROOT version

1. Run `make clean`.

    **Note**: This will delete everything inside `gen` and the `results` folder

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
3. Repeat the processed described in [the previous section](#how-to-run-this-demo)

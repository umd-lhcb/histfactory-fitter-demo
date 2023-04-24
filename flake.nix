{
  description = "Phoebe's demo R(D*) HistFactory fitter";

  inputs = {
    root-curated.url = "github:umd-lhcb/root-curated/dev";
    nixpkgs.follows = "root-curated/nixpkgs";
    flake-utils.follows = "root-curated/flake-utils";
  };

  outputs = { self, nixpkgs, flake-utils, root-curated }:
    flake-utils.lib.eachDefaultSystem (system:
      let
        pkgs = import nixpkgs {
          inherit system;
          overlays = [ root-curated.overlay ];
        };
      in
      {
        devShell = pkgs.mkShell {
          name = "histfactory-fitter-demo";
          buildInputs = with pkgs; [
            # Utilities
            clang-tools
            clang-format-all
            cachix

            # Third-party build dependencies
            cxxopts
            libyamlcpp
            boost

            # Pick your favorite version of ROOT here
            # ROOT 6.28 stack
            root_6_28_02

            # ROOT 6.24 stack
            #root_6_24_02

            # ROOT 6.16 stack, plotting is bugged for this version
            #root_6_16_00

            # ROOT 5.34 stack
            #root_5_34_38
          ];

          shellHook = ''
            export PATH=$(pwd)/gen:$(pwd)/tools:$PATH
          '';
        };
      });
}

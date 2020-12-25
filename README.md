# mrAiHouOceanShader

This project derives from [mrAiVexShader](https://github.com/mruegenberg/mrAiVexShader) to render Houdini Ocean spectra directly in Arnold, without exporting displacement maps or similar.

## Prerequisites
- An installation of Houdini
- The right compiler version matching your Houdini. You can find this in `Help > About Houdini > Show Details > Build platform`.  
  For Linux, this would be GCC 6.3 in the case of Houdini 18.5 (though Clang/LLVM also appears to work fine), for Windows, you'll need to install Visual Studio (probably with the Windows 10 C++ SDK), for OS X you'll need Xcode. For details, see the [Houdini HDK documentation](https://www.sidefx.com/docs/hdk/_h_d_k__intro__compiling.html#HDK_Intro_Compiling_Windows).

## Compilation/Installation
1. Get the Arnold SDK for your system and Arnold version from [SolidAngle](https://www.solidangle.com/arnold/download/) and put it in `deps`.
2. Adjust the path to match your Arnold SDK and Houdini versions in `compile.sh` / `compile.bat` (depending on your system, you just need to modify the .bat (Windows) or .sh (everywhere else))
3. Run the script for your OS from the command line
4. Copy `ai_ocean_samplelayers.dll/.so/.dylib` to the `dso` directory in your Arnold installation (or wherever your Arnold looks for shaders). Same with the `.mtd` file from the `src` folder (this provides the shader UI). It's easiest to put both somewhere in your `ARNOLD_PLUGIN_PATH` (and potentially add `[ARNOLD_PLUGIN_PATH]` to the *Procedural Path* in the System section of your Arnold settings).
5. Start Houdini with Arnold and have some fun. (It's theoretically even possible to use this without Houdini, e.g in Maya or C4D, but you will probably use that to author the ocean spectra anyway.)

# Usage

1. Cache your ocean spectra to disk with the regular Houdini workflow for this. 
2. In you `Arnold Shader Network` or `Arnold Material Builder`, create a new `Ai Ocean Samplelayers` node.
3. Connect its output to the `input` slot of a `Vector Map` node with `Tangent Space` disabled, and connect that to the `displacement` slot on your `Material Output` node.
4. The `a` output of `Ai Ocean Samplelayers` corresponds to the `cusp` output of Houdini's `Ocean Sample Layers` node and can be used for shading

## Examples

See the [blog post](https://www.marcelruegenberg.com/blog/2020/9/9/houdini-arnold-ocean-spectra)

# Status and Limitations

## Limitations
- At the moment, the Arnold render still starts significantly slower than the Houdini version, probably due to differences in renderer architecture/displacement performance.
- No possibility to export `velocity` or `cuspdir`. This is due to Arnold shaders always having only one output.
- the _Anti-Alias Blur_ parameter is not supported. To achieve a similar result, just reduce the maximum subdivision iterations. 

## Future Plans

- possibly provide better support for cusp by creating a separate shading node which outputs the value (and gets it from the main shader via [message passing](https://docs.arnoldrenderer.com/api/arnold-6.0.3.1/group__ai__shader__message.html#details)

- create an auxiliary shader to output bump/normal maps for reducing the amount of subdivisions needed

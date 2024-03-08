//
//     ,ad888ba,                              88
//    d8"'    "8b
//   d8            88,dba,,adba,   ,aPP8A.A8  88     The Cmajor Toolkit
//   Y8,           88    88    88  88     88  88
//    Y8a.   .a8P  88    88    88  88,   ,88  88     (C)2024 Cmajor Software Ltd
//     '"Y888Y"'   88    88    88  '"8bbP"Y8  88     https://cmajor.dev
//                                           ,88
//                                        888P"
//
//  The Cmajor project is subject to commercial or open-source licensing.
//  You may use it under the terms of the GPLv3 (see www.gnu.org/licenses), or
//  visit https://cmajor.dev to learn about our commercial licence options.
//
//  CMAJOR IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL WARRANTIES, WHETHER
//  EXPRESSED OR IMPLIED, INCLUDING MERCHANTABILITY AND FITNESS FOR PURPOSE, ARE
//  DISCLAIMED.

#pragma once

namespace generate_javascript
{

inline std::string generateJavascriptWorklet (cmaj::Patch& patch, const cmaj::Patch::LoadParams& loadParams, bool useBinaryen)
{
    static constexpr const auto generatedModuleSourceText = R"(
//==============================================================================
//
//  This file contains a Javascript/Webassembly/WebAudio export of the Cmajor
//  patch 'PATCH_FILE'.
//
//  This file was auto-generated by the Cmajor toolkit.
//
//  To use it, import this module into your HTML/Javascript code and call
//  `createAudioWorkletNodePatchConnection()` and
//  `connectPatchToDefaultAudioAndMIDI()` to create an instance of the
//  patch and to connect it to the web audio and MIDI devices.
//
//  For more details about Cmajor, visit https://cmajor.dev
//
//==============================================================================

import * as helpers from "./cmaj_api/cmaj_audio_worklet_helper.js"


//==============================================================================
/** This exports the patch's manifest, in case a caller needs to find out about its properties.
 */
export const manifest =
MANIFEST_JSON;

/** Returns the patch's output endpoint list */
export function getOutputEndpoints() { return MAIN_CLASS_NAME.prototype.getOutputEndpoints(); }

/** Returns the patch's input endpoint list */
export function getInputEndpoints()  { return MAIN_CLASS_NAME.prototype.getInputEndpoints(); }

//==============================================================================
/**  Creates an audio worklet node for the patch with the given name, attaches it
 *   to the audio context provided, and returns an object containing the node
 *   and a PatchConnection class to control it.
 *
 *   @param {AudioContext} audioContext - a web audio AudioContext object
 *   @param {string} workletName - the name to give the new worklet that is created
 *   @returns {Object} an object containing the new AudioWorkletNode and PatchConnection
 */
export async function createAudioWorkletNodePatchConnection (audioContext, workletName)
{
  const node = await helpers.createAudioWorkletNode (MAIN_CLASS_NAME, audioContext, workletName, Date.now() & 0x7fffffff);
  const connection = new helpers.AudioWorkletPatchConnection (node, manifest);

  if (manifest.worker?.length > 0)
  {
    connection.readResource = async (path) =>
    {
      return fetch (path);
    };

    connection.readResourceAsAudioData = async (path) =>
    {
      const response = await connection.readResource (path);
      const buffer = await audioContext.decodeAudioData (await response.arrayBuffer());

      let frames = [];

      for (let i = 0; i < buffer.length; ++i)
        frames.push ([]);

      for (let chan = 0; chan < buffer.numberOfChannels; ++chan)
      {
        const src = buffer.getChannelData (chan);

        for (let i = 0; i < buffer.length; ++i)
          frames[i].push (src[i]);
      }

      return {
        frames,
        sampleRate: buffer.sampleRate
      }
    };

    import (connection.getResourceAddress (manifest.worker)).then (module =>
    {
      module.default (connection);
    });
  }

  return { node, connection };
}

//==============================================================================
/**  Takes an audio node and connection that were returned by `createAudioWorkletNodePatchConnection()`
 *   and attempts to hook them up to the default audio and MIDI channels.
 *
 *   @param {AudioWorkletNode} node - the audio node
 *   @param {PatchConnection} connection - the PatchConnection object created by `createAudioWorkletNodePatchConnection()`
 *   @param {AudioContext} audioContext - a web audio AudioContext object
 */
export async function connectPatchToDefaultAudioAndMIDI (node, connection, audioContext)
{
  helpers.connectDefaultAudioAndMIDI ({ node, connection, audioContext, patchInputList: getInputEndpoints() });
}

CMAJOR_WRAPPER_CLASS

)";

    auto wrapper = generateCodeAndCheckResult (patch, loadParams, useBinaryen ? "javascript-binaryen" : "javascript", {});
    auto manifest = choc::text::trim (choc::json::toString (loadParams.manifest.manifest, true));

    return choc::text::trim (choc::text::replace (generatedModuleSourceText,
                                                  "PATCH_FILE", loadParams.manifest.manifestFile,
                                                  "CMAJOR_WRAPPER_CLASS", wrapper.generatedCode,
                                                  "MAIN_CLASS_NAME", wrapper.mainClassName,
                                                  "MANIFEST_JSON", manifest)) + "\n";
}

//==============================================================================
inline GeneratedFiles generateWebAudioHTML (cmaj::Patch& patch, const cmaj::Patch::LoadParams& loadParams, bool useBinaryen)
{
    GeneratedFiles generatedFiles;

    auto& manifest = loadParams.manifest;

    // NB: had weird problems in some cases serving files that began with an underscore.
    auto cleanedName = choc::javascript::makeSafeIdentifier (loadParams.manifest.name);
    auto patchModuleFile = "cmaj_" + std::string (choc::text::trimCharacterAtStart (cleanedName, '_')) + ".js";

    std::string patchLink, patchDesc;

    if (auto url = manifest.manifest["URL"]; url.isString() && ! url.toString().empty())
        patchLink = choc::text::replace (R"(<a href="URL" class=cmaj-small-text>URL</a>)", "URL", url.toString());

    if (! manifest.description.empty() && manifest.description != manifest.name)
        patchDesc = "<span class=cmaj-small-text>" + manifest.description + "</span>";

    auto readme = choc::text::trimStart (choc::text::replace (R"(
### Auto-generated HTML & Javascript for Cmajor Patch "PATCH_NAME"

This folder contains some self-contained HTML/Javascript files that play and show a Cmajor
patch using WebAssembly and WebAudio.

For `index.html` to display correctly, this folder needs to be served as HTTP, so if you're
running it locally, you'll need to start a webserver that serves this folder, and then
point your browser at whatever URL your webserver provides. For example, you could run
`python3 -m http.server` in this folder, and then browse to the address it chooses.

The files have all been generated using the Cmajor command-line tool:
```
cmaj generate --target=webaudio --output=<location of this folder> <path to the .cmajorpatch file to convert>
```

- `index.html` is a minimal page that creates the javascript object that implements the patch,
   connects it to the default audio and MIDI devices, and displays its view.
- `CMAJOR_PATCH_JS` - this is the Javascript wrapper class for the patch, encapsulating its
   DSP as webassembly, and providing an API that is used to both render the audio and
   control its properties.
- `cmaj_api` - this folder contains javascript helper modules and resources.

To learn more about Cmajor, visit [cmajor.dev](cmajor.dev)
)",
                    "PATCH_NAME", manifest.name,
                    "CMAJOR_PATCH_JS", patchModuleFile));

    auto html = choc::text::trimStart (choc::text::replace (R"html(
<!DOCTYPE html><html lang="en">
<head>
  <meta charset="utf-8" />
  <title>Cmajor Patch</title>
</head>

<body>
  <div id="cmaj-main">
    <div id="cmaj-header-bar">
      <div id="cmaj-header-content">
        <svg id="cmaj-header-logo" xmlns="http://www.w3.org/2000/svg" viewBox="150 140 1620 670">
          <g><path d="M944.511,462.372V587.049H896.558V469.165c0-27.572-13.189-44.757-35.966-44.757-23.577,0-39.958,19.183-39.958,46.755V587.049H773.078V469.165c0-27.572-13.185-44.757-35.962-44.757-22.378,0-39.162,19.581-39.162,46.755V587.049H650.4v-201.4h47.551v28.77c8.39-19.581,28.771-32.766,54.346-32.766,27.572,0,46.353,11.589,56.343,35.166,11.589-23.577,33.57-35.166,65.934-35.166C918.937,381.652,944.511,412.42,944.511,462.372Zm193.422-76.724h47.953v201.4h-47.953V557.876c-6.794,19.581-31.167,33.567-64.335,33.567q-42.558,0-71.928-29.969c-19.183-20.381-28.771-45.155-28.771-75.128s9.588-54.743,28.771-74.726c19.581-20.377,43.556-30.366,71.928-30.366,33.168,0,57.541,13.985,64.335,33.566Zm3.6,100.7c0-17.579-5.993-32.368-17.981-43.953-11.589-11.59-26.374-17.583-43.559-17.583s-31.167,5.993-42.756,17.583c-11.187,11.585-16.783,26.374-16.783,43.953s5.6,32.369,16.783,43.958c11.589,11.589,25.575,17.583,42.756,17.583s31.97-5.994,43.559-17.583C1135.537,518.715,1141.53,503.929,1141.53,486.346Zm84.135,113.49c0,21.177-7.594,29.571-25.575,29.571-2.8,0-7.192-.4-13.185-.8v42.357c4.393.8,11.187,1.2,19.979,1.2,44.355,0,66.734-22.776,66.734-67.932V385.648h-47.953Zm23.978-294.108c-15.987,0-28.774,12.385-28.774,28.372s12.787,28.369,28.774,28.369a28.371,28.371,0,0,0,0-56.741Zm239.674,104.694c21.177,20.381,31.966,45.956,31.966,75.924s-10.789,55.547-31.966,75.928-47.154,30.769-77.926,30.769-56.741-10.392-77.922-30.769-31.966-45.955-31.966-75.928,10.789-55.543,31.966-75.924,47.154-30.768,77.922-30.768S1468.136,390.041,1489.317,410.422Zm-15.585,75.924c0-17.981-5.994-32.766-17.985-44.753-11.988-12.39-26.773-18.383-44.356-18.383-17.981,0-32.766,5.993-44.754,18.383-11.589,11.987-17.583,26.772-17.583,44.753s5.994,32.77,17.583,45.156c11.988,11.987,26.773,17.985,44.754,17.985q26.374,0,44.356-17.985C1467.738,519.116,1473.732,504.331,1473.732,486.346Zm184.122-104.694c-28.373,0-50.349,12.787-59.941,33.964V385.648h-47.551v201.4h47.551v-105.9c0-33.169,21.177-53.948,54.345-53.948a102.566,102.566,0,0,1,19.979,2V382.85A74.364,74.364,0,0,0,1657.854,381.652ZM580.777,569.25l33.909,30.087c-40.644,47.027-92.892,70.829-156.173,70.829-58.637,0-108.567-19.737-149.788-59.8C268.082,570.31,247.763,519.8,247.763,460s20.319-109.726,60.962-149.786c41.221-40.059,91.151-60.38,149.788-60.38,62.119,0,113.789,22.643,154.432,68.507l-33.864,30.134c-16.261-19.069-35.272-32.933-56.978-41.783V486.346H496.536V621.1Q546.954,610.231,580.777,569.25Zm-237.74,9.1A150.247,150.247,0,0,0,396.5,614.04V486.346H370.929V319.387a159.623,159.623,0,0,0-27.892,22.829Q297.187,389.16,297.186,460C297.186,507.229,312.47,547.06,343.037,578.354Zm115.476,46.66a187.178,187.178,0,0,0,27.28-1.94V486.346H474.548V295.666c-5.236-.426-10.567-.677-16.035-.677a177.387,177.387,0,0,0-40.029,4.4V486.346H407.239v131.4A175.161,175.161,0,0,0,458.513,625.014Z" fill="#fff" /></g>
        </svg>
        <span id="cmaj-header-title">PATCH_NAME</span>
      </div>
      <div id="cmaj-header-close-button"></div>
    </div>
    <div id="cmaj-outer-container">
      <div id="cmaj-inner-container">
        <div id="cmaj-start-area">
          <div id="cmaj-info">
            PATCH_DESC
            PATCH_LINK
            <span style="margin-top: 3rem">- Click to Start -</span>
          </div>
        </div>
      </div>
    </div>
  </div>
</body>

<style>
    * { box-sizing: border-box; padding: 0; margin: 0; border: 0; }
    html { background: black; overflow: hidden; }
    body { padding: 0.5rem; display: block; position: absolute; width: 100%; height: 100%; }

    #cmaj-main {
      display: flex;
      flex-direction: column;
      color: white;
      font-family: Monaco, Consolas, monospace;
      width: 100%;
      height: 100%;
    }

    #cmaj-outer-container {
      display: block;
      position: relative;
      width: 100%;
      height: 100%;
      overflow: auto;
    }

    #cmaj-inner-container {
      position: relative;
      width: 100%;
      height: 100%;
      overflow: visible;
      transform-origin: 0% 0%;
    }

    #cmaj-header-bar { position: relative; height: 1.5rem; min-height: 1.5rem; overflow: hidden; display: flex; align-items: center; margin-bottom: 2px; user-select: none; }
    #cmaj-header-content { height: 100%; overflow: hidden; display: flex; align-items: center; flex-grow: 1; }
    #cmaj-header-logo { height: 100%; fill-opacity: 0.8; }
    #cmaj-header-title { height: 100%; flex-grow: 1; text-align: center; padding-right: 1rem; }

    #cmaj-header-close-button { display: inline-block; height: 100%; width: 2rem; opacity: 0.3; position: relative; }
    #cmaj-header-close-button:hover  { opacity: 1; }
    #cmaj-header-close-button:before, #cmaj-header-close-button:after { position: absolute; left: 1rem; content: ' '; height: 100%; width: 2px; background-color: #fff; }
    #cmaj-header-close-button:before { transform: rotate(45deg); }
    #cmaj-header-close-button:after { transform: rotate(-45deg); }

    #cmaj-start-area { border: none;
                       background-color: transparent;
                       width: 100%;
                       height: 100%;
                       display: flex;
                       flex-direction: column;
                       justify-content: center;
                       align-items: center;
    }

    #cmaj-start-area * { padding: 0.5rem; }
    #cmaj-start-area a { color: rgb(120, 120, 184); }

    #cmaj-info { display: flex; flex-direction: column; justify-content: center; align-items: center; flex-grow: 1; }

    .cmaj-small-text { font-size: 75%; }
</style>

<script type="module">

import * as patch from "./PATCH_MODULE_FILE"
import { createPatchView, scalePatchViewToFit } from "./cmaj_api/cmaj-patch-view.js"

//==============================================================================
async function loadPatch()
{
    const audioContext = new AudioContext();

    const { node, connection }
        = await patch.createAudioWorkletNodePatchConnection (audioContext, "cmaj-worklet-processor");

    await patch.connectPatchToDefaultAudioAndMIDI (node, connection, audioContext);

    const view = await createPatchView (connection);

    if (view)
    {
        const outer = document.getElementById ("cmaj-outer-container");
        const inner = document.getElementById ("cmaj-inner-container");
        const headerBar = document.getElementById ("cmaj-header-bar");
        const headerContent = document.getElementById ("cmaj-header-content");
        const closeButton = document.getElementById ("cmaj-header-close-button");

        const originalHTML = inner.innerHTML;
        inner.innerHTML = "";
        inner.appendChild (view);

        const originalHeaderHeight = headerBar.style.height;
        const originalHeaderContent = headerContent.innerHTML;

        if (view?.hasOwnTitleBar?.())
        {
            headerContent.innerHTML = "";
            headerBar.style.height = "0.8rem";
            headerBar.style.minHeight = "0.8rem";
        }

        const resizeObserver = new ResizeObserver (() => scalePatchViewToFit (view, inner, outer));
        resizeObserver.observe (outer);

        scalePatchViewToFit (view, inner, outer);

        closeButton.style.opacity = 0.4;

        closeButton.onclick = () =>
        {
            audioContext?.close();
            inner.innerHTML = originalHTML;
            headerContent.innerHTML = originalHeaderContent;
            headerBar.style.height = originalHeaderHeight;
            headerBar.style.minHeight = originalHeaderHeight;
            inner.style.transform = "none";
            attachLoadHandler();
        }
    }
}

function attachLoadHandler()
{
    const closeButton = document.getElementById ("cmaj-header-close-button");
    closeButton.style.opacity = 0;
    document.getElementById ("cmaj-start-area").onclick = loadPatch;
}

attachLoadHandler();

</script>
</html>
)html",
                    "PATCH_MODULE_FILE", patchModuleFile,
                    "PATCH_NAME", manifest.name,
                    "PATCH_DESC", patchDesc,
                    "PATCH_LINK", patchLink));

    // NB keep this file first in the list
    generatedFiles.addFile (patchModuleFile, generateJavascriptWorklet (patch, loadParams, useBinaryen));
    generatedFiles.addFile ("index.html", html);
    generatedFiles.addFile ("README.md", readme);

    for (auto& f : cmaj::EmbeddedWebAssets::files)
        generatedFiles.addFile ("cmaj_api/" + std::string (f.name), std::string (f.content));

    generatedFiles.addFile ("cmaj_api/cmaj_audio_worklet_helper.js", cmaj::EmbeddedAssets::getInstance().findContent ("cmaj_audio_worklet_helper.js"));

    auto workletHelperModuleSourceText = cmaj::EmbeddedAssets::getInstance().findContent ("cmaj_audio_worklet_helper.js");

    auto manifestFilePath = std::filesystem::path (manifest.manifestFile);
    auto manifestFilename = manifestFilePath.filename();
    auto fullPathToManifest = std::filesystem::path (manifest.getFullPathForFile (manifestFilename.string()));
    auto manifestFolder = fullPathToManifest.parent_path();

    for (auto& view : manifest.views)
    {
        auto viewFolder = std::filesystem::path (manifest.getFullPathForFile (view.getSource())).parent_path();
        generatedFiles.findAndAddFiles (viewFolder, manifestFolder);
    }

    for (auto& resource : manifest.resources)
        generatedFiles.findAndAddFiles (manifestFolder, manifestFolder, choc::text::WildcardPattern (resource, false));

    if (! loadParams.manifest.patchWorker.empty())
        generatedFiles.readAndAddFile (manifestFolder / GeneratedFiles::trimLeadingSlash (loadParams.manifest.patchWorker), manifestFolder);

    return generatedFiles;
}

}

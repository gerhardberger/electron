// This file provides the global, process and Buffer variables to internal
// Electron code once they have been deleted from the global scope.
//
// It does this through the ProvidePlugin in the webpack.config.base.js file
// Check out the Module.wrapper override in renderer/init.ts for more
// information on how this works and why we need it

// Rip global off of window (which is also global) so that webpack doesn't
// auto replace it with a looped reference to this file

// @AROUND: Addressing Worklet case properly, where no window or self exists.
let _global: NodeJS.Global | undefined;
try {
  _global = ((self as any) || (window as any)).global as NodeJS.Global;
} catch {}

const process = _global ? _global.process : undefined;
const Buffer = _global ? _global.Buffer : undefined;

export {
  _global,
  process,
  Buffer
}

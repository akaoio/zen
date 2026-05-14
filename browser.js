import ZEN from "./zen.min.js";
import "./lib/webrtc.min.js";
import "./lib/radisk.min.js";
import "./lib/radix.min.js";
import "./lib/opfs.min.js";

export const BROWSER_RUNTIME = {
	core: true,
	axe: true,
	webrtc: true,
	radisk: true,
	radix: true,
	opfs: typeof navigator !== "undefined" && !!navigator.storage?.getDirectory,
	notes: {
		axe: "AXE is now built into `zen.min.js` core bundle — no separate import needed.",
	},
};

globalThis.ZEN = ZEN;
globalThis.ZEN_BROWSER_RUNTIME = BROWSER_RUNTIME;

export default ZEN;

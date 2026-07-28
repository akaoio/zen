/**
 * Worker realm for test/zen/port.js — a real ZEN instance in a separate
 * thread, attached to the main realm over a transferred MessagePort.
 */
import { parentPort } from "node:worker_threads";
import ZEN from "../../../zen.js";

const zen = new ZEN({
  peers: [],
  WebSocket: false,
  localStorage: false,
  radisk: false,
  rfs: false,
  stats: false,
  axe: false,
  multicast: false,
});

parentPort.once("message", (port) => {
  zen.attach(port);
  zen.get("worker-room").get("hello").put("from-worker");
  parentPort.postMessage("attached");

  zen.get("worker-room")
    .get("question")
    .on((value) => {
      if (value !== undefined) parentPort.postMessage(value);
    });
});

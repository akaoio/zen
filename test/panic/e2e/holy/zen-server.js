import panic from "panic-client";
import ports from "./ports";
import Zen from "../../../zen.js";
import http from "http";

var server = new http.Server();
var zen = new Zen({
  file: "delete-me.json",
  web: server,
});

server.listen(ports.zen);

panic.server("http://localhost:" + ports.panic);

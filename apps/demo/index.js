import { once } from "node:events";
import path from "node:path";
import { fileURLToPath } from "node:url";
import express from "express";
import shotium from "@shotkit/shotium";

const demoDir = path.dirname(fileURLToPath(import.meta.url));
const siteDir = path.resolve(demoDir, "..", "genshin-faruzan-profile-card");
const outputPath = path.join(demoDir, "faruzan.png");

const app = express();
app.use(express.static(siteDir));

const server = app.listen(0, "127.0.0.1");
await once(server, "listening");

const pageUrl = `http://127.0.0.1:${server.address().port}/index.html`;

try {
  await new Promise((resolve) => setTimeout(resolve, 200));
  shotium.start({ cacheDir: null });

  const { stats } = await shotium.screenshot({
    file: pageUrl,
    selector: "#card",
    // pageGotoParams: { waitUntil: "networkidle" }, // 这个打开无论任何条件 都会增加500ms等待时间
    path: outputPath,
  });

  console.log(`截图地址: ${pageUrl}`);
  console.log(
    `渲染耗时: ${stats.timing.render}ms, 总耗时: ${stats.timing.total}ms`,
  );
} finally {
  await shotium.stop();
  const closed = once(server, "close");
  server.close();
  server.closeAllConnections();
  await closed;
}

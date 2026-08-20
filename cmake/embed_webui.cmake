# Turn src/app/webui/index.html into a .cpp byte array so the console UI
# ships inside the ax_pipeline_app binary (GET / serves it directly).
file(READ "${IN}" _hex HEX)
string(REGEX REPLACE "([0-9a-f][0-9a-f])" "0x\\1," _bytes "${_hex}")
file(WRITE "${OUT}" "// Auto-generated from src/app/webui/index.html - do not edit.
namespace axpipeline::app {
extern const unsigned char kWebuiHtml[];
extern const unsigned long kWebuiHtmlLen;
const unsigned char kWebuiHtml[] = {${_bytes}0x00};
const unsigned long kWebuiHtmlLen = sizeof(kWebuiHtml) - 1;
}  // namespace axpipeline::app
")

// Colors Ruva code blocks, since the site's highlighter has no Ruva grammar.

var RUVA_KEYWORDS = /^(launch|keep|vary|share|bring|craft|give|object|check|otherwise|choose|when|walk|from|until|in|during)$/;
var RUVA_TYPES = /^(int|real|bool|text|void|file)$/;
var RUVA_CONSTANTS = /^(true|false)$/;
var RUVA_BUILTINS = /^(ask|show|toInt|toReal|toText|toBool|push|pop|length|fileWrite|fileAppend|fileRead|fileExists|fileOpen|fileReadFrom|fileWriteTo|fileClose)$/;
var RUVA_TOKENS = /(\/\/[^\n]*)|("(?:[^"\\\n]|\\.)*")|(\b\d+(?:\.\d+)?\b)|([A-Za-z_][A-Za-z0-9_]*)/g;

function escapeHtml(text) {
  return text.replace(/&/g, "&amp;").replace(/</g, "&lt;").replace(/>/g, "&gt;");
}

function tokenClass(match, source, end) {
  var word = match[0];

  if (match[1])
    return "c1";

  if (match[2])
    return "s2";

  if (match[3])
    return "mi";

  if (RUVA_KEYWORDS.test(word))
    return "k";

  if (RUVA_TYPES.test(word))
    return "kt";

  if (RUVA_CONSTANTS.test(word))
    return "kc";

  if (RUVA_BUILTINS.test(word))
    return "nb";

  if (source.charAt(end) === "(")
    return "nf";

  return null;
}

function colorBlock(block) {
  var source = block.textContent;
  var html = "";
  var last = 0;
  var match;

  RUVA_TOKENS.lastIndex = 0;

  while ((match = RUVA_TOKENS.exec(source)) !== null) {
    var name = tokenClass(match, source, RUVA_TOKENS.lastIndex);

    html += escapeHtml(source.slice(last, match.index));
    html += name ? '<span class="' + name + '">' + escapeHtml(match[0]) + "</span>" : escapeHtml(match[0]);
    last = RUVA_TOKENS.lastIndex;
  }

  block.innerHTML = html + escapeHtml(source.slice(last));
  block.dataset.ruva = "colored";
}

document$.subscribe(function () {
  document.querySelectorAll(".language-ruva code").forEach(function (block) {
    if (!block.dataset.ruva)
      colorBlock(block);
  });
});

var mutationTimerID = -1;
const mutationObserver = new MutationObserver(function() {
    if (mutationTimerID == -1) {
        mutationTimerID = setTimeout(function () {
            mutationTimerID = -1;
            doc = document.documentElement;
            window.webkit.messageHandlers.bijiben.postMessage({
                messageName: 'ContentsUpdate',
                outerHTML: doc.outerHTML,
                innerText: doc.innerText
            });
        }, 0);
    }
});
mutationObserver.observe(document, { childList: true, characterData: true, subtree: true });

function rangeHasText(range) {
    if (range.startContainer.nodeType == Node.TEXT_NODE)
        return true;
    if (range.endContainer.nodeType == Node.TEXT_NODE)
        return true;

    node = range.cloneContents();
    while (node) {
        if (node.nodeType == Node.TEXT_NODE)
            return true;

        if (node.hasChildNodes())
            node = node.firstChild;
        else if (node.nextSlibling)
            node = node.nextSlibling;
        else {
            node = node.parentNode;
            if (node)
                node = node.nextSlibling;
        }
    }

    return false;
};

function findParent(node, tag) {
    var element = node;
    if (node.nodeType != Node.ELEMENT_NODE)
        element = node.parentElement;
    if (element)
        return element.closest(tag);
    return null;
}

function rangeBlockFormat(range) {
    node = range.startContainer;
    if (findParent(node, "ul"))
        return "UL";
    if (findParent(node, "ol"))
        return "OL";
    return "NONE";
};

var selectionChangeTimerID = -1;
document.addEventListener('selectionchange', function () {
    if (selectionChangeTimerID == -1) {
        selectionChangeTimerID = setTimeout(function () {
            selectionChangeTimerID = -1;
            selection = window.getSelection();
            if (selection.rangeCount < 1)
                return;
            range = selection.getRangeAt(0);
            window.webkit.messageHandlers.bijiben.postMessage({
                messageName: 'SelectionChange',
                hasText: rangeHasText(range),
                text: range.toString(),
                blockFormat: rangeBlockFormat(range)
            });
        }, 0);
    }
}, false);

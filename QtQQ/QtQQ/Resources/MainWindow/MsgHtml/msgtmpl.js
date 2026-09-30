function appendHtml(html) {
    $("#placeholder").append(html);
    window.scrollTo(0, document.body.scrollHeight);
}
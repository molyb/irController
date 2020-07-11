$(document).ready(function() {
    $.ajax({
        type:     'GET',
        url:      'config',
        dataType: 'json',
    }).done(function (data) {
        console.log(data["title"])
        $('#message').text(data["title"]);
    }).fail(function(jqxhr, status, error) {
        alert(status + " : " + error);
    });
});


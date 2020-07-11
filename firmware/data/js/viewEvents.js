function createCheckbox(hour, minute, name, value) {
    return "<p> <input type = \'checkbox\' name=\'event\' value=\'" + value + "\'>"
        + hour + ":"  + minute + " " + name + "</p>";
}

$(function() {
    $.ajax({
        type:     'GET',
        url:      'config',
        dataType: 'json',
    }).done(function (data) {
        for (const index in data["events"]) {
            const elem = data["events"][index]
            $('#event_pos').append(createCheckbox(elem["hour"], elem["minute"], elem["function_name"], index));
        }
    }).fail(function(jqxhr, status, error) {
        alert(status + " : " + error);
    });
});


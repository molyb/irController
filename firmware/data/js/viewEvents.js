function createCheckbox(hour, minute, name, weekday, value) {
    let weekday_disp = ""
    weekday.forEach(function(event_request) {
        console.log(event_request)
        if (event_request === "true") {
            weekday_disp += "*";
        } else {
            weekday_disp += "-";
        }
    })
    return "<p> <input type = \'checkbox\' name=\'delete_index\' value=\'" + value + "\'>"
        + hour + ":"  + minute + " " + name + " " + weekday_disp + "</p>";
}

$(function() {
    $.ajax({
        type:     'GET',
        url:      'config',
        dataType: 'json',
    }).done(function (data) {
        for (const index in data["events"]) {
            const elem = data["events"][index]
            $('#event_pos').append(
                createCheckbox(elem["hour"], elem["minute"], elem["function_name"], elem["weekday"], index));
        }
    }).fail(function(jqxhr, status, error) {
        alert(status + " : " + error);
    });
});


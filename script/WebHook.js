function doGet(e) {
  return handleAttendance(e);
}

function handleAttendance(e) {
  try {
    var params = e.parameter;

    var studentID = params.id     || "Unknown";
    var name      = params.name   || "N/A";
    var roll      = params.roll   || "N/A";
    var branch    = params.branch || "N/A";

    var timestamp = new Date().toLocaleString("en-IN", {
      timeZone: "Asia/Kolkata",
      year: "numeric", month: "2-digit", day: "2-digit",
      hour: "2-digit", minute: "2-digit", second: "2-digit",
      hour12: false
    });

    var sheet = SpreadsheetApp.getActiveSheet();
    var data = sheet.getDataRange().getValues();

    var lastStatus = "";  // Empty = first time

    for (var i = data.length - 1; i >= 1; i--) {
      if (String(data[i][1]).trim() === studentID.trim()) {
        lastStatus = (data[i][5] || "").trim().toLowerCase();
        break;
      }
    }

    var newStatus = "Entry";

    if (lastStatus !== "") {
      if (lastStatus === "entry") {
        newStatus = "Exit";
      } else if (lastStatus === "exit") {
        newStatus = "Entry";
      }
    }

    sheet.appendRow([timestamp, studentID, name, roll, branch, newStatus]);

    return ContentService.createTextOutput("OK - " + newStatus + " recorded")
                         .setMimeType(ContentService.MimeType.TEXT);

  } catch (err) {
    Logger.log(err);
    return ContentService.createTextOutput("Error: " + err.message)
                         .setMimeType(ContentService.MimeType.TEXT);
  }
}
var express = require('express'); /*express for handlebars*/
var handlebars = require('express-handlebars'); /* templating engine */
var bodyParser = require('body-parser');
const path = require("path"); /* allows use of path.join for folder paths */
const http = require('http'); /* make internal requests to control service */
var app = express();

/* Connection settings */
var conn = require('./js/global/connection.js');

app.engine('handlebars', handlebars({
  defaultLayout: 'main',
}));


app.use(bodyParser.urlencoded({ extended: true }));

app.use('/static', express.static(path.join(__dirname, 'public'))); /* public folder */
app.use('/js', express.static(path.join(__dirname, 'js'))); /* Javascript folder */
app.use('/css', express.static(path.join(__dirname, 'css'))); /* style sheets folder */
app.use('/img', express.static(path.join(__dirname, 'img'))); /* images folder */


// Handlebars
app.set('view engine', 'handlebars');
app.set('port', process.argv[2]); /* sets port to what is given in command line */


// Get requests
app.use('/plant', require('./js/plant.js'));
app.use('/profiles', require('./js/profiles.js'));
app.use('/zones', require('./js/zones.js'));
app.use('/ledStates', require('./js/ledStates.js'));
app.use('/dailyStates', require('./js/dailyStates.js'));


// Update requests
app.post('/ledStates', function (req, res) {
  let id = req.body.id;
  console.log("PATCH ledstate " + id);

  // Build JSON for control service
  let j = {};
  j['r'] = parseInt(req.body.red);
  j['g'] = parseInt(req.body.green);
  j['b'] = parseInt(req.body.blue);
  j['intensity'] = parseInt(req.body.intensity);
  j['power'] = !!req.body.power; // Black magic. body.power comes back as "on" or undefined. (!undefined = true) and (!"on" = false) so (bool value = !!body.power)

  // Patch control service
  var options = {
    "method": "PATCH",
    "hostname": conn.hostname,
    "port": conn.port,
    "path": conn.ledStates + "/" + id + "/edit",
    "headers": {
      "content-type": conn.header
    }
  };
  
  // Make request
  let request = http.request(options, function (response) {
    if (response.statusCode != 200) {
      let error = "Error! Status: " + res.statusCode + ", Response: " + String(response.body);
      console.log(error);
      console.log(response.body);
    }
    else {
      console.log("Response:" + response.body);
      res.redirect('back');
    }
  });
  request.write(JSON.stringify(j));
  request.end();
});


// Add requests


// Delete requests



// Bad states
app.use(function (req, res) {
  res.status(404);
  res.render('404');
});

app.use(function (req, res) {
  res.status(500);
  res.render('500');
});


// Listen
app.listen(app.get('port'), function () {
  console.log('Express started on http://localhost:' + app.get('port') + '; press Ctrl-C to terminate.');
});

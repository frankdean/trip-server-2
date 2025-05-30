// -*- mode: js2; fill-column: 80; indent-tabs-mode: nil; c-basic-offset: 2; -*-
// vim: set tw=80 ts=2 sts=0 sw=2 et ft=js norl:
/*
    This file is part of Trip Server 2, a program to support trip recording and
    itinerary planning.

    Copyright (C) 2022-2025 Frank Dean <frank.dean@fdsd.co.uk>

    This program is free software: you can redistribute it and/or modify it
    under the terms of the GNU Affero General Public License as published by the
    Free Software Foundation, either version 3 of the License, or (at your
    option) any later version.

    This program is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Affero General Public License
    for more details.

    You should have received a copy of the GNU Affero General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

// console.log(zxcvbn('Tr0ub4dour&3'));
const pwinput = document.getElementById('new-password');
pwinput.addEventListener('input', (event) => {
  const pw = pwinput.value;
  // console.log('Password', pw);
  const div = document.getElementById('crack-time-div');
  const progressMeter = document.getElementById('pw-strength');
  const inputScore = document.getElementById('pw-score');
  const saveButton = document.getElementById('btn-save');
  const warning = document.getElementById('feedback-warning');
  const suggestions = document.getElementById('feedback-suggestions');
  if (pw.length > 0) {
    const result = zxcvbn(pw);
    // console.log('Password strength', result.score);
    const crackTime = result.crack_times_seconds.online_no_throttling_10_per_second;
    // console.log('Crack time', crackTime);
    // div.className = crackTime < 10000 ? 'd-block' : 'd-none';
    div.className = 'd-block';
    progressMeter.className = 'd-block';
    inputScore.value = result.score;
    switch(result.score) {
    case 0:
      saveButton.className = 'btn btn-lg btn-success disabled';
      break;
    case 1:
      saveButton.className = 'btn btn-lg btn-success disabled';
      progressMeter.className = 'progress-bar w-25 bg-danger';
      break;
    case 2:
      saveButton.className = 'btn btn-lg btn-success disabled';
      progressMeter.className = 'progress-bar w-50 bg-warning';
      break;
    case 3:
      saveButton.className = 'btn btn-lg btn-success';
      progressMeter.className = 'progress-bar w-75 bg-info';
      break;
    case 4:
      saveButton.className = 'btn btn-lg btn-success';
      progressMeter.className = 'progress-bar w-100 bg-success';
      break;
    }
    const span = document.getElementById('crack-time');
    const seconds_per_year = 31557600;
    const seconds_per_century = seconds_per_year * 100;
    const seconds_per_month = seconds_per_year / 12;
    if (crackTime >= seconds_per_century)
      span.innerHTML = (crackTime / seconds_per_century).toFixed(1) + ' centuries';
    else if (crackTime >= seconds_per_year)
      span.innerHTML = (crackTime / seconds_per_year).toFixed(1) + ' years';
    else if (crackTime >= seconds_per_month)
      span.innerHTML = (crackTime / seconds_per_month).toFixed(1) + ' months';
    else if (crackTime >= 86400)
      span.innerHTML = (crackTime / 86400).toFixed(1) + ' days';
    else if (crackTime >= 3600)
      span.innerHTML = (crackTime / 3600).toFixed(1) + ' hours';
    else if (crackTime >= 60)
      span.innerHTML = (crackTime / 60).toFixed(1) + ' minutes';
    else
      span.innerHTML = crackTime + ' seconds';
    // console.log(result.score, result.feedback);
    if (result.score <= 2) {
      if (result.feedback.warning.length > 0) {
        warning.innerHTML = '<p>' + result.feedback.warning + '</p>';
        warning.className = 'alert alert-danger d-block';
      }
      if (result.feedback.suggestions.length > 0) {
        let html = '<ul>';
        result.feedback.suggestions.forEach((s) => {
          html += '<li>' + s + '</li>';
        });
        html += '</ul>';
        suggestions.innerHTML = html;
        // console.debug('Adding', html);
        suggestions.className = 'alert alert-warning d-block';
      }
    } else {
      suggestions.className = 'd-none';
      warning.className = 'd-none';
    }
  } else {
    div.className = 'd-none';
    warning.className = 'd-none';
    suggestions.className = 'd-none';
    inputScore.value = '0';
    saveButton.className = 'btn btn-lg btn-success disabled';
    // progressMeter.className = 'd-none';
    // console.debug('no change');
  }
}, false);

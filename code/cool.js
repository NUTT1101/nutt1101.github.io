var iRadio = 0;
var iInput = 0;
var radioDone = false;
var inputDone = false;

function checkComplete() {
    if (radioDone && inputDone) {
        document.querySelector(".office-form-submit-button-div").children[0].click();
    }
}

function stopInterval(intervalId, setDone) {
    clearInterval(intervalId);
    setDone(true);
    checkComplete();
}

var intervalRadio = setInterval(function() {
    let radio = document.getElementsByClassName("office-form-matrix-radio")[iRadio]?.children[0];
    if (!radio) {
        stopInterval(intervalRadio, (done) => radioDone = done);
        return;
    }
    radio.click();
    iRadio += 5;
}, 1);

var intervalInput = setInterval(function(){
  let input = document.querySelectorAll('.office-form-textfield input')[iInput];
  if (!input) {
      stopInterval(intervalInput, (done) => inputDone = done);
      return;
  }
  input.value = iInput === 0 ? '100' : '0';
  input.dispatchEvent(new Event('input'));
  input.dispatchEvent(new Event('change'));
  document.querySelector('.SkipBtn').click();
  iInput += 1;
}, 200);

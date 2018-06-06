/* global Pebble */

var MessageQueue = (function(Pebble){

/**
 * MessageQueue is an app message queue that guarantees delivery and order.
 */
var MessageQueue = function() {
  this._queue = [];
  this._sending = false;

  this._consume = this.consume.bind(this);
  this._cycle = this.cycle.bind(this);
  this._failure = this.failure.bind(this);
};

MessageQueue.prototype.stop = function() {
  this._sending = false;
};

MessageQueue.prototype.consume = function() {
  this._queue.splice(0, 1);
  if (this._queue.length === 0) {
    console.log("consume: stop");
    return this.stop();
  }
  this.cycle();
};

/*
var otc = function(obj, name) {
    console.log("===== Object : " + name + " === ");
    for (var key in obj) {
        console.log(name + "['" + key + "'] = " + obj[key]);
    }
};
*/


MessageQueue.prototype.failure = function(e) {
  /*
  if (e) {
    otc(e, "Error in failure:");
    if (e.data) {
      otc(e.data, "Error.data:");
    }
    // console.log('Unable to deliver message with transactionId=' +
    //   e.data.transactionId +
    //   ' Error is: ' + e.error.message);
  }
  */
  setTimeout(this._cycle, 1000);
};



MessageQueue.prototype.cycle = function(e) {
  /*
  if (e) {
    otc(e, "Error in cycle:");
    if (e.data) {
      otc(e.data, "Error.data:");
    }
    // console.log('Unable to deliver message with transactionId=' +
    //   e.data.transactionId +
    //   ' Error is: ' + e.error.message);
  }
  */
  if (!this._sending) {
    return;
  }
  var head = this._queue[0];
  if (!head) {
    console.log("cycle: stop");
    return this.stop();
  }
  console.log("cycle: _queue.length = " + this._queue.length);
  console.log("cycle: sendAppMessage: " + JSON.stringify(head));
  Pebble.sendAppMessage(head, this._consume, this._failure);
};

MessageQueue.prototype.send = function(message) {
  this._queue.push(message);
  if (this._sending) {
    return;
  }
  this._sending = true;
  this.cycle();
};

return MessageQueue;

})(Pebble);
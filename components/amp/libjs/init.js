globalThis=new Function("return this;")(),globalThis.process=system,function(t){var n,e=t.Promise,o=e&&"resolve"in e&&"reject"in e&&"all"in e&&"race"in e&&(new e(function(t){n=t}),"function"==typeof n);"undefined"!=typeof exports&&exports?(exports.Promise=o?e:T,exports.Polyfill=T):"function"==typeof define&&define.amd?define(function(){return o?e:T}):o||(t.Promise=T);function r(){}var i="pending",f="sealed",c="fulfilled",u="rejected";function a(t){return"[object Array]"===Object.prototype.toString.call(t)}var s,h="undefined"!=typeof setImmediate?setImmediate:setTimeout,l=[];function p(){for(var t=0;t<l.length;t++)l[t][0](l[t][1]);s=!(l=[])}function d(t,n){l.push([t,n]),s||(s=!0,h(p,1))}function y(t,n){function e(t){b(n,t)}try{t(function(t){_(n,t)},e)}catch(t){e(t)}}function w(t){var n=t.owner,e=n.state_,o=n.data_,r=t[e],i=t.then;if("function"==typeof r){e=c;try{o=r(o)}catch(t){b(i,t)}}m(i,o)||(e===c&&_(i,o),e===u&&b(i,o))}function m(n,e){var o;try{if(n===e)throw new TypeError("A promises callback cannot return that same promise.");if(e&&("function"==typeof e||"object"==typeof e)){var t=e.then;if("function"==typeof t)return t.call(e,function(t){o||(o=!0,(e!==t?_:v)(n,t))},function(t){o||(o=!0,b(n,t))}),1}}catch(t){return o||b(n,t),1}}function _(t,n){t!==n&&m(t,n)||v(t,n)}function v(t,n){t.state_===i&&(t.state_=f,t.data_=n,d(j,t))}function b(t,n){t.state_===i&&(t.state_=f,t.data_=n,d(P,t))}function g(t){var n=t.then_;t.then_=void 0;for(var e=0;e<n.length;e++)w(n[e])}function j(t){t.state_=c,g(t)}function P(t){t.state_=u,g(t)}function T(t){if("function"!=typeof t)throw new TypeError("Promise constructor takes a function argument");if(this instanceof T==!1)throw new TypeError("Failed to construct 'Promise': Please use the 'new' operator, this object constructor cannot be called as a function.");this.then_=[],y(t,this)}T.prototype={constructor:T,state_:i,then_:null,data_:void 0,then:function(t,n){var e={owner:this,then:new this.constructor(r),fulfilled:t,rejected:n};return this.state_===c||this.state_===u?d(w,e):this.then_.push(e),e.then},catch:function(t){return this.then(null,t)}},T.all=function(c){if(!a(c))throw new TypeError("You must pass an array to Promise.all().");return new this(function(e,t){var o=[],r=0;function n(n){return r++,function(t){o[n]=t,--r||e(o)}}for(var i,f=0;f<c.length;f++)(i=c[f])&&"function"==typeof i.then?i.then(n(f),t):o[f]=i;r||e(o)})},T.race=function(r){if(!a(r))throw new TypeError("You must pass an array to Promise.race().");return new this(function(t,n){for(var e,o=0;o<r.length;o++)(e=r[o])&&"function"==typeof e.then?e.then(t,n):t(e)})},T.resolve=function(n){return n&&"object"==typeof n&&n.constructor===this?n:new this(function(t){t(n)})},T.reject=function(e){return new this(function(t,n){n(e)})}}("undefined"!=typeof window?window:"undefined"!=typeof global?global:"undefined"!=typeof self?self:this);
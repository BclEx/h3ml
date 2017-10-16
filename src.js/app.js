import Promise from 'promise-polyfill';
if (!window.Promise) {
    window.Promise = Promise;
}
import Doc from './files/Doc';

document.global = function () {
    let doc = new Doc();
    console.log('doc', doc);
    doc.render();
};
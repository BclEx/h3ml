export default class {
    _frame = null;
    _doc = null;

    load = (url, cb) => {
        let frame = this._frame = document.createElement('iframe');
        let self = this;
        frame.addEventListener('load', function () {
            try { self._doc = this.contentWindow.document; }
            // Note that SecurityError exception is specific to Firefox.
            catch (e) { if (e.name == 'SecurityError') { console.log('SecurityError.'); } }
            cb();
        });
        frame.style.display = 'none';
        frame.src = url;
        document.body.appendChild(frame);
    }

    getStyles = () => {
        let styleSheets = this._doc.styleSheets;
        for (let sheetIdx = 0; sheetIdx < styleSheets.length; sheetIdx++) {
            let rules = styleSheets[sheetIdx].rules || styleSheets[sheetIdx].cssRules;
            for (let x = 0; x < rules.length; x++) {
                let cssText = rules[x].cssText ? rules[x].cssText : rules[x].style.cssText;
                let style = rules[x].style;
                // for (let i = style.length; i--;) {
                //     let nameString = style[i];
                //     let nameValue = style.getPropertyValue(nameString);
                //     console.log(nameString, nameValue);
                // }
                console.log('cssText', cssText, style);
            }
        }
    }

    render() {
        let url = '/test/simple.html';
        this.load(url, () => { //?v=${new Date().getTime()}
            this.getStyles();
        });
    }
}
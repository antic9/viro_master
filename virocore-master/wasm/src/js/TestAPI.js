/* Generated from Java with JSweet 2.0.0 - http://www.jsweet.org */
var viro;
(function (viro) {
    /**
     * Simple example.
     * @class
     */
    var TestAPI = (function () {
        function TestAPI() {

        }
        /**
         * Test Javadoc.
         * @param {string} arg
         */
        TestAPI.prototype.callNativeMethod = function (arg) {
            Module.nativeTestMethod(this);
        };
        TestAPI.prototype.callJSMethod = function (number1, number2) {
            alert("JS method was called with number " + number1 + " and " + number2);
        };

        return TestAPI;
    }());
    viro.TestAPI = TestAPI;
    TestAPI["__class"] = "viro.TestAPI";
})(viro || (viro = {}));

var Module = {
    onRuntimeInitialized: function() {
        // Run initialization code here
    }
};
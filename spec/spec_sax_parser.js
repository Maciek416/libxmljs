process.mixin(require('./helpers'));

describe("SAX Push Parser", function() {
  var callbacks = {};
  var filename = path.dirname(__filename)+'/fixtures/sax_parser_test.xml';

  function createParser(parserType) {
    parser = new libxml[parserType](function(cb) {
      function argsToArray(args) {
        var ary = [];
        var i;
        for (i = 0; i < args.length; i++)
          ary.push(args[i]);
        return ary;
      };

      cb.onStartDocument(function() {
        callbacks.startDocument.push(argsToArray(arguments));
      });

      cb.onEndDocument(function() {
        callbacks.endDocument.push(argsToArray(arguments));
      });

      cb.onStartElementNS(function(elem, attrs, prefix, uri, namespaces) {
        callbacks.startElementNS.push(argsToArray(arguments));
      });

      cb.onEndElementNS(function(elem, prefix, uri) {
        callbacks.endElementNS.push(argsToArray(arguments));
      });

      cb.onCharacters(function(chars) {
        if (chars[0] && !chars[0].match(/^[\s\n\r]+$/))
          callbacks.characters.push(argsToArray(arguments));
      });

      cb.onCdata(function(cdata) {
        callbacks.cdata.push(argsToArray(arguments));
      });

      cb.onComment(function(msg) {
        callbacks.comment.push(argsToArray(arguments));
      });

      cb.onWarning(function(msg) {
        callbacks.warning.push(argsToArray(arguments));
      });

      cb.onError(function(msg) {
        callbacks.error.push(argsToArray(arguments));
      });
    });
    return parser;
  };

  beforeEach(function() {
    callbacks = clone(callbackTest);
  });

  it('will properly parse a regular string', function() {
    var str = posix.cat(filename).wait();
    var parser = createParser('SaxParser');
    parser.parseString(str);
    assertEqual(JSON.stringify(callbackControl), JSON.stringify(callbacks));
  });

  it('will properly parse a string chunk by chunk', function() {
    var str_ary = posix.cat(filename).wait().split("\n");
    var parser = createParser('SaxPushParser');
    var i;
    for (i = 0; i < str_ary.length; i++)
      parser.push(str_ary[i], (i+1 == str_ary.length));

    var control = clone(callbackControl);
    control.error = [["Extra content at the end of the document\n"]];
    assertEqual(JSON.stringify(control), JSON.stringify(callbacks));
  });

  it('will properly parse a file', function() {
    var parser = createParser('SaxParser');
    parser.parseFile(filename);

    var control = clone(callbackControl);
    assertEqual(JSON.stringify(control), JSON.stringify(callbacks));
  });

  it('can can be reused as a string parser', function() {
    var str = posix.cat(filename).wait();
    var parser = createParser('SaxParser');
    
    for (var i=0; i<10; i++)
      parser.parseString(str);

  });

  it('can can be reused as a string parser', function() {
    var parser = createParser('SaxParser');
    
    for (var i=0; i<10; i++)
      parser.parseFile(filename);

  });
});

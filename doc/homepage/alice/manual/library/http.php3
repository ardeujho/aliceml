<?php include("macros.php3"); ?>
<?php heading("The Http structure",
	      "The <TT>Http</TT> structure") ?>

<?php section("synopsis", "synopsis") ?>

  <PRE>
    signature HTTP
    structure Http : HTTP
  </PRE>

  <P>
    This structure serves to represent and analyze requests and responses
    conforming to the HTTP protocol.  The current implementation supports
    a subset of HTTP Version 1.1 as specified by RFC&nbsp;2616.
  </P>

<?php section("import", "import") ?>

  <PRE>
    import signature HTTP from "x-alice:/lib/system/HTTP-sig"
    import structure Http from "x-alice:/lib/system/Http"
  </PRE>

<?php section("interface", "interface") ?>

  <PRE>
    signature HTTP =
    sig
	structure <A href="#StringMap">StringMap</A> : IMP_MAP where type key = string

	type <A href="#request">request</A> =
	     {method : string, uri : Url.t, protocol : string,
	      headers : string StringMap.t, body : string}
	type <A href="#response">response</A> =
	     {protocol : string, statusCode : int, reasonPhrase : string,
	      headers : string StringMap.t, body : string}
	type <A href="#simple_response">simple_response</A> =
	     {statusCode : int, contentType : string, body : string}

	exception <A href="#Closed">Closed</A>
	exception <A href="#Format">Format</A>

	val <A href="#readRequest">readRequest</A> : Socket.socket -> request
	val <A href="#readResponse">readResponse</A> : Socket.socket -> response

	val <A href="#writeRequest">writeRequest</A> : Socket.socket * request -> unit
	val <A href="#writeResponse">writeResponse</A> : Socket.socket * response -> unit

	val <A href="#reasonPhrase">reasonPhrase</A> : int -> string
	val <A href="#makeResponse">makeResponse</A> : simple_response -> response
    end
  </PRE>

<?php section("description", "description") ?>

  <DL>
    <DT>
      <TT>structure <A name="StringMap">StringMap</A> :
	IMP_MAP where type key = string</TT>
    </DT>
    <DD>
      <P>A structure used for representing HTTP headers, which are mappings
	from strings (names) to strings (values).</P>
    </DD>

    <DT>
      <TT>type <A name="request">request</A> =
     {method : string, uri : Url.t, protocol : string,
      headers : string StringMap.t, body : string}</TT>
    </DT>
    <DD>
      <P>The type of HTTP requests.  Represents a request with <TT>method</TT>
	for <TT>uri</TT> using protocol version <TT>protocol</TT>.  The type
	of the <TT>body</TT> document, if non-empty, must be given as a header
	in <TT>headers</TT>.</P>
    </DD>

    <DT>
      <TT>type <A name="response">response</A> =
     {protocol : string, statusCode : int, reasonPhrase : string,
      headers : string StringMap.t, body : string}</TT>
    </DT>
    <DD>
      <P>The type of HTTP responses.  Represents a response with
	<TT>statusCode</TT> and <TT>reasonPhrase</TT>, using protocol
	version <TT>protocol</TT>.  The type of the <TT>body</TT> document,
	if non-empty, must be given as a header in <TT>headers</TT>.</P>
    </DD>

    <DT>
      <TT>type <A name="simple_response">simple_response</A> =
     {statusCode : int, contentType : string, body : string}</TT>
    </DT>
    <DD>
      <P>The type of simplified HTTP responses.  Represents a response
	with <TT>statusCode</TT>, carrying document <TT>body</TT>
	of type <TT>contentType</TT>.  Can be extended to a full
	<TT><A href="#response">response</A></TT> using the function
	<TT><A href="#makeResponse">makeResponse</A></TT>.</P>
    </DD>

    <DT>
      <TT>exception <A name="Closed">Closed</A></TT>
    </DT>
    <DD>
      <P>indicates that the socket connection was closed while the functions
	in this structure attempted to read an HTTP request or response.</P>
    </DD>

    <DT>
      <TT>exception <A name="Format">Format</A></TT>
    </DT>
    <DD>
      <P>indicates that the data received while attempting to read an HTTP
	request or response from a socket did not conform to the HTTP
	protocol, Version 1.1.</P>
    </DD>

    <DT>
      <TT><A name="readRequest">readRequest</A> <I>sock</I></TT>
    </DT>
    <DD>
      <P>attempts to read an HTTP request from <I>sock</I>, parses it,
	and returns it.  Raises <TT><A href="#Closed">Closed</A></TT>
	or <TT><A href="#Format">Format</A></TT> if the connection was
	closed or the request was malformed, respectively.</P>
    </DD>

    <DT>
      <TT><A name="readResponse">readResponse</A> <I>sock</I></TT>
    </DT>
    <DD>
      <P>attempts to read an HTTP response from <I>sock</I>, parses it,
	and returns it.  Raises <TT><A href="#Closed">Closed</A></TT>
	or <TT><A href="#Format">Format</A></TT> if the connection was
	closed or the response was malformed, respectively.</P>
    </DD>

    <DT>
      <TT><A name="writeRequest">writeRequest</A>
	(<I>sock</I>, <I>request</I>)</TT>
    </DT>
    <DD>
      <P>formats <I>request</I> according to the HTTP protocol, Version 1.1,
	and writes it to <I>sock</I>.</P>
    </DD>

    <DT>
      <TT><A name="writeResponse">writeResponse</A>
	(<I>sock</I>, <I>response</I>)</TT>
    </DT>
    <DD>
      <P>formats <I>response</I> according to the HTTP protocol, Version 1.1,
	and writes it to <I>sock</I>.</P>
    </DD>

    <DT>
      <TT><A name="reasonPhrase">reasonPhrase</A> <I>code</I></TT>
    </DT>
    <DD>
      <P>returns a reason phrase describing status <I>code</I>.
	Implemented to use the reason phrases given in RFC&nbsp;2616,
	or <TT>"Unknown"</TT> if <I>code</I> does not correspond to
	a status code defined in RFC&nbsp;2616.</P>
    </DD>

    <DT>
      <TT><A name="makeResponse">makeResponse</A> <I>simpleResponse</I></TT>
    </DT>
    <DD>
      <P>returns a full response corresponding to the response specified
	by <I>simpleResponse</I>.  The <TT>headers</TT> only specify
	<TT>Content-Type</TT> and <TT>Content-Length</TT>, <TT>protocol</TT>
	is fixed to <TT>HTTP/1.1</TT>, and <TT>reasonPhrase</TT> is
	generated using function <TT><A href="#reasonPhrase">reasonPhrase</A
	></TT>.</P>
    </DD>
  </DL>

<?php section("also", "see also") ?>

  <DL><DD>
    <A href="url.php3"><TT>Url</TT></A>,
    <A href="http-client.php3"><TT>HttpClient</TT></A>,
    <A href="http-server.php3"><TT>HttpServer</TT></A>
  </DD></DL>

<?php footing() ?>

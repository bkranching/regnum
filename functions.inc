<?php
include("regnum.cfg");
date_default_timezone_set('Etc/UTC');

function sec_session_start() 
{
	$session_name = 'regnum_session';   // Set a custom session name 
	$secure = SECURE; // This stops JavaScript being able to access the session id.
	$httponly = true; // Forces sessions to only use cookies.
	if (ini_set('session.use_only_cookies', 1) === FALSE) 
	{
		display_error("Could not initiate a safe session (ini_set)");
	}
	$cookieParams = session_get_cookie_params(); // Gets current cookies params.
	session_set_cookie_params($cookieParams["lifetime"], $cookieParams["path"], $cookieParams["domain"], $secure, $httponly);
	session_name($session_name); // Sets the session name to the one set above.
	session_start();            // Start the PHP session 
	session_regenerate_id();    // regenerated the session, delete the old one. 
}

function login_check () 
{
	(isset($_SESSION['user_id'], $_SESSION['username'], $_SESSION['login_string'])) ?: return 0;

	$user_id = $_SESSION['user_id'];
	$login_string = $_SESSION['login_string'];
	$username = $_SESSION['username'];
	$user_browser = $_SERVER['HTTP_USER_AGENT'];

	$dbh = database_open();
	$stmt = $dbh->prepare("SELECT password FROM users WHERE id = :id LIMIT 1");
	$stmt->bindParam(':id', $user_id);
	$stmt->execute();
	$results = $stmt->fetch(PDO::FETCH_ASSOC);

	if ( $results['password'] ) 
	{
		$login_check = hash('sha512', $password . $user_browser);
		hash_equals($login_check, $login_string) ? return 1 : return 0;
	} 
	else 
	{
		return 0;
	}
}

function database_open()
{
	global $db_server, $db_port, $db_database, $db_user, $db_pass;
	try 
	{
		$dbh = new PDO ( "pgsql:host=$db_server;port=$db_port;dbname=$db_database", $db_user, $db_pass );
		return $dbh;
	} 
	catch (PDOException $e) 
	{
		print "<b>Error!: " . $e->getMessage() . "</b><br/>";
		die();
	}
}

function valid_domain ($domain, ...&$errs)
{
	global $specialNamesOpenNIC, $specialNamesRFC6761, $specialNamesIANA, $specialNamesDNS;
	$domain = strtolower($domain);
	if ( ! preg_match('/^[a-z0-9\-_]+(\.[a-z0-9\-_]+)*$/', $domain) )
	{
		$errs[0] = "Domain names may only include a-z0-9.-_";
		return 0;
	}
	if ( strlen($domain) <= 0 || strlen($domain) > 255 ) // Domain name labels limit=63 and uri limit=253 octects see RFC1035
	{
		$errs[0] = "Sorry, domain names must contain at least 1 character and be no longer than 253 characters.";
		return 0;
	}
	if ( preg_match('/[a-z0-9\-_]{64}/', $domain)
	{
		$errs[0] = "Each label in a domain name must be 63 or fewer characters";
		return 0;
	}
	if(preg_match('/^[a-zA-Z]{2}$/', $name)) 
	{
		$errs[0] = "ISO 3166 country codes are reserved at this time.";
		return 0;
	}
	if(preg_match($specialNamesRFC6761, $name)) 
	{
		$errs[0] = "Domain names must not contain special DNS names as specified in RFC6761.";
		return 0;
	}
	if(preg_match($specialNamesIANA, $name)) 
	{
		$errs[0] = "Sorry, we do not want to mess with ICANN or IANA special DNS names.<br>\n"; 
		$errs[0] .= "See: http://www.icann.org/en/about/agreements/registries/unsponsored/registry-agmt-appk-26apr01-en.htm<br>\n";
		return 0;
	}
	if ( preg_match($specialNamesOpenNIC, $domain) || preg_match($specialNamesDNS, $domain) )
	{
		$errs[0] = "That domain is reserved by administration";
		return 0;
	}
	return 1;	// user is trying to take an special name
}

function domain_taken($domain)
{
	valid_domain($domain) ?: return 0;

	$base = database_open();
	$stmt = $dbh->prepare("SELECT * FROM domains WHERE domain=':domain' LIMIT 1");
	$stmt->bindParam(':domain', $domain);
	$stmt->execute();
	$results = $stmt->fetch();

	$results['domain'] == $domain ? return 1 : return 0;
}

function valid_username ($username)
{
	preg_match("/^[0-9a-zA-Z]+?$/", $username) ? return 1 : return 0;
}

function username_taken ($username)
{
	#if ( ! valid_username($username) ) { return 1; } 
	$base = database_open();
	$stmt = $dbh->prepare("SELECT username FROM users WHERE username=':user' LIMIT 1");
	$stmt->bindParam(':user', $username);
	$stmt->execute();
	$results = $stmt->fetch();

	if($results) { return 1; } 
	else { return 0; }
}

function show_header()
{
	global $ws_title, $TLD;
	echo "<html>\n<head>\n<title>".$ws_title."</title>\n</head>\n<body>\n";
	echo "<p align=\"left\"><a href=\"index.php\">opennic.".$TLD."</a></p>\n";
	if(!isset($_SESSION['username']))
	{
		echo "<p align=\"right\">Already have an account? <a href=\"user.php?action=frm_login\">Log in</a> or <a href=\"user.php?action=frm_register\">Register</a></p>\n";
	} else {
		echo "<p align=\"right\">Hello <b style=\"color: red;\">".$_SESSION['username']."</b>! [<a href=\"user.php\">My Account</a>]&nbsp;[<a href=\"logout.php\">Logout</a>]</p>\n";
	}
}

function show_footer ()
{
	global $sw_version;
	echo "<center><font size=\"-1\">Powered by MUD4TLD v".$sw_version."</font></center>\n"; 
	echo "</body>\n</html>\n";
}

function display_error ($string)
{
	echo "Error: $string <br>\n";
	show_footer();
	die();
}

//function to validate ip address format in php by Roshan Bhattarai(http://roshanbh.com.np)
function validateIPAddress($ip_addr)
{
	//echo "Debug IP validation: ip_addr = '$ip_addr'<br>";
	// first of all the format of the ip address is matched
	if(preg_match("/^(\d{1,3})\.(\d{1,3})\.(\d{1,3})\.(\d{1,3})$/",$ip_addr))
	{
		// now all the intger values are separated
		$parts=explode(".",$ip_addr);
		// now we need to check each part can range from 0-255
		foreach($parts as $ip_parts)
		{
			if(intval($ip_parts)>255 || intval($ip_parts)<0)
				return 0; // if number is not within range of 0-255
		}
		return 1;
	}
	elseif(preg_match("/^(?:[0-9a-fA-F]{1,4}:){7}[0-9a-fA-F]{1,4}$/", $ip_addr))
	{
		//check for invalid addresses
		if(preg_match("/^[fF][eE]80:|^[fF]{2}00:|^0{1,4}:/", $ip_addr))
		{
			return 0;
		}
		return 1;
	}
	elseif($ip_addr == "") { return 1; }
	return 0; // if format of ip address doesn't matches
}

function validateFQDN($hostname, $domain)
{
	global $TLD;
	if(preg_match("/^(?:[a-zA-Z0-9]+?(?:[a-zA-Z0-9\-]+?[a-zA-Z0-9])?\.)?[a-zA-Z0-9]+?(?:[a-zA-Z0-9\-]+?[a-zA-Z0-9])?\.[a-zA-Z0-9]+?(?:[a-zA-Z0-9\-]+?[a-zA-Z0-9])?$/", $hostname))
	{
		if(preg_match("/$domain.$TLD$/", $hostname))
		{
			return 1;
		}
	}
	elseif($hostname == "") { return 1; }
	return 0;	
}

function validateDSKEY($dskey)
{
	if(preg_match("/^[0-9a-zA-Z\.\-\s]*?$/", $dskey)) { return 1; }
	return 0;
}

function validateTXT($txt)
{
	if(preg_match("/^[0-9a-zA-Z\.\-\s,_]*?$/", $txt)) { return 1; }
	return 0;
}

function validatePGP($pgpkey)
{
	if(preg_match("^[0-9a-zA-Z\.\-\s\n\r=+:/]*?$", $txt)) { return 1; }
	return 1;
}

function generate_password($length = 8) 
{
    $chars = 'abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789';
    $count = mb_strlen($chars);
    for ($i = 0, $result = ''; $i < $length; $i++) {
        $index = rand(0, $count - 1);
        $result .= mb_substr($chars, $index, 1);
    }
    return $result;
}

function crypt_pass ($plaintext)
{
	return password_hash($plaintext, PASSWORD_DEFAULT);
}

?>

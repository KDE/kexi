
		</div>
	      </div>
	    </div>
	
	<div id="left">

	    <div class="menutitle"><div>
		    <h2 id="cp-menu-login">Login</h2>
		    <a href="#cp-skip-login" class="cp-doNotDisplay">Skip menu "Login"</a>
            </div></div>
            <ul>
		<form action="/login" method="get">
                    <input style="width: 90%;" type="text" name="user" value="root"/>
                    <input style="width: 90%;" type="text" name="pass" value="root"/>
                    <input type="submit" value="Login"/>
                    <input type="reset" value="Reset"/>
		</form>
            </ul>
            <a name="cp-skip-login"/>
	    
	    <div class="menu_box">
		<a name="cp-menu" />
		<div class="menutitle"><div>
		  <h2 id="cp-menu-tables">Tables</h2>
		  <a href="#cp-skip-tables" class="cp-doNotDisplay">Skip menu "Tables"</a>
		</div></div>
		<ul>
		  {{TABLE_LIST}}
		</ul>
		<a name="cp-skip-tables"/>

        <div class="menutitle"><div>
            <h2 id="cp-menu-queries">Queries</h2>
            <a href="#cp-skip-queries" class="cp-doNotDisplay">Skip menu "Queries"</a>
        </div></div>
        <ul>
            {{QUERY_LIST}}
        </ul>
        <a name="cp-skip-queries"/>

		<!--
		    <div class="menutitle"><div>
		    <h2 id="cp-menu-download">Download</h2>
		    <a href="#cp-skip-download" class="cp-doNotDisplay">Skip menu "Download"</a>
		    </div></div>
		    <ul>
		    <li>
		    <a href="/download/">Stable Version</a>
		    </li>
		    <li>
		    <a href="http://techbase.kde.org/Getting_Started">Source Code</a>
		    </li>
		    <li>
		    <a href="/mirrors/">FTP Mirrors</a>
		    </li>
		    </ul>
		-->

		<a name="cp-skip-explore"/>
		<div class="cp-doNotDisplay">
		  <h2>Global navigation links</h2>
		  <ul>
		    <li><a href="http://www.kde.org/" accesskey="8">KDE Home</a></li>
		    <li><a href="http://accessibility.kde.org/" accesskey="9">KDE Accessibility Home</a></li>
		    <li><a href="/media/accesskeys.php" accesskey="0">Description of Access Keys</a></li>
		    <li><a href="#cp-content">Back to content</a></li>
		    <li><a href="#cp-menu">Back to menu</a></li>
		  </ul>
		</div>
	      </div>
	    </div>
	    <div class="clearer"></div>
	  </div>
	  <div class="clearer"></div>
	</div>
	<div id="end_body"></div>

	<div id="footer"><div id="footer_text">
	  Template Maintained by Lorenzo Villani - Based upon the template created for KDE.org website<br/>
	  KDE<sup>&#174;</sup> and <a href="/media/images/kde_gear_black.png">the K Desktop Environment<sup>&#174;</sup> logo</a> are registered trademarks of 				<a href="http://ev.kde.org/" title="Homepage of the KDE non-profit Organization">KDE e.V.</a> |
	  <a href="http://www.kde.org/contact/impressum.php">Legal</a>
	</div></div>
      </div>

      <!--
	  WARNING: DO NOT SEND MAIL TO THE FOLLOWING EMAIL ADDRESS! YOU WILL
	  BE BLOCKED INSTANTLY AND PERMANENTLY!
	  <a href="mailto:aaaatrap-485cfab9973109f2@kde.org">Block me</a>
	  WARNING END
      -->

    </body>
  </html>

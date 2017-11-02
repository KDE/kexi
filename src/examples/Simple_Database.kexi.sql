PRAGMA foreign_keys=OFF;
BEGIN TRANSACTION;
CREATE TABLE kexi__db (db_property Text(32), db_value CLOB);
INSERT INTO "kexi__db" VALUES('kexidb_major_ver','1');
INSERT INTO "kexi__db" VALUES('kexidb_minor_ver','2');
INSERT INTO "kexi__db" VALUES('kexiproject_major_ver','1');
INSERT INTO "kexi__db" VALUES('kexiproject_minor_ver','0');
CREATE TABLE kexi__fields (t_id UNSIGNED Integer, f_type UNSIGNED Byte, f_name Text(200), f_length Integer, f_precision Integer, f_constraints Integer, f_options Integer, f_default Text(200), f_order Integer, f_caption Text(200), f_help CLOB);
INSERT INTO "kexi__fields" VALUES(1,3,'id',0,0,119,1,NULL,1,'ID',NULL);
INSERT INTO "kexi__fields" VALUES(1,3,'age',0,0,0,1,NULL,2,'Age',NULL);
INSERT INTO "kexi__fields" VALUES(1,11,'name',200,0,0,0,NULL,3,'Name',NULL);
INSERT INTO "kexi__fields" VALUES(1,11,'surname',200,0,0,0,NULL,4,'Surname',NULL);
INSERT INTO "kexi__fields" VALUES(53,11,'a',200,0,0,200,'1',0,NULL,NULL);
INSERT INTO "kexi__fields" VALUES(53,11,'b',200,0,0,200,'2',1,NULL,NULL);
INSERT INTO "kexi__fields" VALUES(2,3,'id',0,0,119,1,NULL,0,'ID',NULL);
INSERT INTO "kexi__fields" VALUES(2,11,'model',200,0,0,0,NULL,1,'Car model',NULL);
INSERT INTO "kexi__fields" VALUES(103,4,'id',0,0,119,1,NULL,0,'Id',NULL);
INSERT INTO "kexi__fields" VALUES(103,3,'owner',0,0,0,0,NULL,1,'Owner',NULL);
INSERT INTO "kexi__fields" VALUES(103,3,'car',0,0,0,0,NULL,2,'Car',NULL);
INSERT INTO "kexi__fields" VALUES(103,3,'since',0,0,0,0,NULL,3,'Since',NULL);
CREATE TABLE kexi__objectdata (o_id UNSIGNED Integer NOT NULL, o_data CLOB, o_sub_id Text(200));
INSERT INTO "kexi__objectdata" VALUES(4,'<!DOCTYPE UI>
<UI version="3.1" stdsetdef="1">
<kfd:customHeader version="2"/>
<pixmapinproject/>
<class>QWidget</class>
<widget class="QWidget">
<property name="name">
<string>formularz1</string>
</property>
<property name="geometry">
<rect>
<x>0</x>
<y>0</y>
<width>500</width>
<height>230</height>
</rect>
</property>
<property name="paletteBackgroundColor">
<color>
<red>97</red>
<green>147</green>
<blue>207</blue>
</color>
</property>
<property name="dataSource">
<string>persons</string>
</property>
<widget class="KexiDBLabel">
<property name="name">
<string>TextLabel1</string>
</property>
<property name="geometry">
<rect>
<x>20</x>
<y>20</y>
<width>292</width>
<height>46</height>
</rect>
</property>
<property name="alignment">
<set>AlignLeading|AlignTop</set>
</property>
<property name="text">
<string>Persons</string>
</property>
<property name="font">
<font>
<family>Verdana</family>
<pointsize>20</pointsize>
<weight>50</weight>
<bold>0</bold>
<italic>0</italic>
<underline>0</underline>
<strikeout>0</strikeout>
</font>
</property>
<property name="dataSource">
<string></string>
</property>
</widget>
<widget class="KexiDBImageBox">
<property name="name">
<string>icon</string>
</property>
<property name="geometry">
<rect>
<x>380</x>
<y>10</y>
<width>90</width>
<height>70</height>
</rect>
</property>
<property name="alignment">
<set>AlignTrailing|AlignTop</set>
</property>
<property name="storedPixmapId">
<number>1</number>
</property>
</widget>
<widget class="KexiDBLineEdit">
<property name="name">
<string>name</string>
</property>
<property name="geometry">
<rect>
<x>100</x>
<y>80</y>
<width>260</width>
<height>30</height>
</rect>
</property>
<property name="dataSource">
<string>name</string>
</property>
</widget>
<widget class="KexiDBLineEdit">
<property name="name">
<string>surname</string>
</property>
<property name="geometry">
<rect>
<x>100</x>
<y>120</y>
<width>260</width>
<height>30</height>
</rect>
</property>
<property name="dataSource">
<string>surname</string>
</property>
</widget>
<widget class="KexiDBLineEdit">
<property name="name">
<string>age</string>
</property>
<property name="geometry">
<rect>
<x>100</x>
<y>160</y>
<width>70</width>
<height>30</height>
</rect>
</property>
<property name="alignment">
<set>AlignTrailing|AlignVCenter</set>
</property>
<property name="dataSource">
<string>age</string>
</property>
</widget>
<widget class="KexiDBLabel">
<property name="name">
<string>label3</string>
</property>
<property name="geometry">
<rect>
<x>20</x>
<y>80</y>
<width>70</width>
<height>30</height>
</rect>
</property>
<property name="alignment">
<set>AlignTrailing|AlignVCenter</set>
</property>
<property name="text">
<string>Name:</string>
</property>
</widget>
<widget class="KexiDBLabel">
<property name="name">
<string>label4</string>
</property>
<property name="geometry">
<rect>
<x>20</x>
<y>120</y>
<width>70</width>
<height>30</height>
</rect>
</property>
<property name="alignment">
<set>AlignTrailing|AlignVCenter</set>
</property>
<property name="text">
<string>Surname:</string>
</property>
</widget>
<widget class="KexiDBLabel">
<property name="name">
<string>label5</string>
</property>
<property name="geometry">
<rect>
<x>20</x>
<y>160</y>
<width>70</width>
<height>30</height>
</rect>
</property>
<property name="alignment">
<set>AlignTrailing|AlignVCenter</set>
</property>
<property name="text">
<string>Age:</string>
</property>
</widget>
</widget>
<layoutDefaults margin="11" spacing="6"/>
<tabstops>
<tabstop>icon</tabstop>
<tabstop>name</tabstop>
<tabstop>surname</tabstop>
<tabstop>age</tabstop>
</tabstops>
</UI>
',NULL);
INSERT INTO "kexi__objectdata" VALUES(65,'<!DOCTYPE UI>
<UI version="3.1" stdsetdef="1">
<kfd:customHeader version="2"/>
<pixmapinproject/>
<class>QWidget</class>
<widget class="QWidget">
<property name="name">
<string>form1</string>
</property>
<property name="geometry">
<rect>
<x>0</x>
<y>0</y>
<width>480</width>
<height>220</height>
</rect>
</property>
<property name="paletteBackgroundColor">
<color>
<red>203</red>
<green>238</green>
<blue>185</blue>
</color>
</property>
<property name="focusPolicy">
<enum>NoFocus</enum>
</property>
<property name="dataSource">
<string>cars</string>
</property>
<widget class="KexiDBLabel">
<property name="name">
<string>TextLabel1</string>
</property>
<property name="geometry">
<rect>
<x>20</x>
<y>20</y>
<width>266</width>
<height>54</height>
</rect>
</property>
<property name="alignment">
<set>AlignLeading|AlignTop</set>
</property>
<property name="text">
<string>Cars</string>
</property>
<property name="font">
<font>
<family>Verdana</family>
<pointsize>20</pointsize>
<weight>50</weight>
<bold>0</bold>
<italic>0</italic>
<underline>0</underline>
<strikeout>0</strikeout>
</font>
</property>
<property name="dataSource">
<string>model</string>
</property>
</widget>
<widget class="KexiDBLabel">
<property name="name">
<string>TextLabel2</string>
</property>
<property name="geometry">
<rect>
<x>20</x>
<y>100</y>
<width>66</width>
<height>30</height>
</rect>
</property>
<property name="alignment">
<set>AlignTrailing|AlignVCenter</set>
</property>
<property name="text">
<string>ID:</string>
</property>
</widget>
<widget class="KexiDBPushButton">
<property name="name">
<string>closeButton</string>
</property>
<property name="geometry">
<rect>
<x>320</x>
<y>100</y>
<width>120</width>
<height>40</height>
</rect>
</property>
<property name="text">
<string>Close window</string>
</property>
<property name="onClickAction">
<string>form:cars</string>
</property>
<property name="onClickActionOption">
<string>close</string>
</property>
<property name="focusPolicy">
<enum>NoFocus</enum>
</property>
</widget>
<widget class="KexiDBPushButton">
<property name="name">
<string>saveButton</string>
</property>
<property name="geometry">
<rect>
<x>320</x>
<y>150</y>
<width>120</width>
<height>40</height>
</rect>
</property>
<property name="text">
<string>Save changes</string>
</property>
<property name="onClickAction">
<string>currentForm:data_save_row</string>
</property>
<property name="focusPolicy">
<enum>NoFocus</enum>
</property>
</widget>
<widget class="KexiDBLabel">
<property name="name">
<string>TextLabel4</string>
</property>
<property name="geometry">
<rect>
<x>20</x>
<y>140</y>
<width>66</width>
<height>34</height>
</rect>
</property>
<property name="alignment">
<set>AlignTrailing|AlignVCenter</set>
</property>
<property name="text">
<string>Model:</string>
</property>
</widget>
<widget class="KexiDBImageBox">
<property name="name">
<string>icon</string>
</property>
<property name="geometry">
<rect>
<x>340</x>
<y>20</y>
<width>100</width>
<height>70</height>
</rect>
</property>
<property name="alignment">
<set>AlignTrailing|AlignTop</set>
</property>
<property name="storedPixmapId">
<number>3</number>
</property>
</widget>
<widget class="KexiDBLineEdit">
<property name="name">
<string>idEdit</string>
</property>
<property name="geometry">
<rect>
<x>90</x>
<y>100</y>
<width>200</width>
<height>31</height>
</rect>
</property>
<property name="text">
<string>id</string>
</property>
<property name="focusPolicy">
<enum>WheelFocus</enum>
</property>
<property name="dataSource">
<string>id</string>
</property>
</widget>
<widget class="KexiDBLineEdit">
<property name="name">
<string>modelEdit</string>
</property>
<property name="geometry">
<rect>
<x>90</x>
<y>140</y>
<width>200</width>
<height>31</height>
</rect>
</property>
<property name="focusPolicy">
<enum>WheelFocus</enum>
</property>
<property name="dataSource">
<string>model</string>
</property>
</widget>
</widget>
<layoutDefaults margin="11" spacing="6"/>
<tabstops>
<tabstop>icon</tabstop>
<tabstop>idEdit</tabstop>
<tabstop>modelEdit</tabstop>
<tabstop>closeButton</tabstop>
<tabstop>saveButton</tabstop>
</tabstops>
</UI>
',NULL);
INSERT INTO "kexi__objectdata" VALUES(104,'SELECT persons.name, persons.surname, persons.age, cars.model, ownership.since FROM ownership, persons, cars WHERE persons.id = ownership.owner AND cars.id = ownership.car ORDER BY persons.surname, ownership.since','sql');
INSERT INTO "kexi__objectdata" VALUES(104,'<query_layout><table name="ownership" x="319" y="15" width="264" height="222"/><table name="persons" x="615" y="20" width="264" height="222"/><table name="cars" x="9" y="8" width="264" height="222"/><conn mtable="persons" mfield="id" dtable="ownership" dfield="owner"/><conn mtable="cars" mfield="id" dtable="ownership" dfield="car"/></query_layout>','query_layout');
INSERT INTO "kexi__objectdata" VALUES(103,'<!DOCTYPE EXTENDED_TABLE_SCHEMA>
<EXTENDED_TABLE_SCHEMA version="1" >
 <field name="owner" >
  <lookup-column>
   <row-source>
    <type>table</type>
    <name>persons</name>
   </row-source>
   <bound-column>
    <number>0</number>
   </bound-column>
   <visible-column>
    <number>3</number>
   </visible-column>
  </lookup-column>
 </field>
 <field name="car" >
  <lookup-column>
   <row-source>
    <type>table</type>
    <name>cars</name>
   </row-source>
   <bound-column>
    <number>0</number>
   </bound-column>
   <visible-column>
    <number>1</number>
   </visible-column>
  </lookup-column>
 </field>
</EXTENDED_TABLE_SCHEMA>
','extended_schema');
INSERT INTO "kexi__objectdata" VALUES(105,'<!DOCTYPE UI>
<UI version="3.1" stdsetdef="1">
<kfd:customHeader version="2"/>
<pixmapinproject/>
<class>QWidget</class>
<widget class="QWidget">
<property name="name">
<string>form1</string>
</property>
<property name="geometry">
<rect>
<x>0</x>
<y>0</y>
<width>514</width>
<height>611</height>
</rect>
</property>
<property name="paletteBackgroundColor">
<color>
<red>154</red>
<green>168</green>
<blue>198</blue>
</color>
</property>
<property name="dataSource">
<string>ownership</string>
</property>
<widget class="KexiDBComboBox">
<property name="name">
<string>comboBox</string>
</property>
<property name="geometry">
<rect>
<x>100</x>
<y>120</y>
<width>221</width>
<height>26</height>
</rect>
</property>
<property name="dataSource">
<string>owner</string>
</property>
</widget>
<widget class="KexiDBComboBox">
<property name="name">
<string>comboBox2</string>
</property>
<property name="geometry">
<rect>
<x>100</x>
<y>160</y>
<width>221</width>
<height>26</height>
</rect>
</property>
<property name="dataSource">
<string>car</string>
</property>
</widget>
<widget class="KexiDBLabel">
<property name="name">
<string>label</string>
</property>
<property name="geometry">
<rect>
<x>20</x>
<y>120</y>
<width>70</width>
<height>30</height>
</rect>
</property>
<property name="alignment">
<set>AlignTrailing|AlignVCenter</set>
</property>
<property name="text">
<string>Owner:</string>
</property>
</widget>
<widget class="KexiDBLabel">
<property name="name">
<string>label2</string>
</property>
<property name="geometry">
<rect>
<x>20</x>
<y>160</y>
<width>70</width>
<height>30</height>
</rect>
</property>
<property name="alignment">
<set>AlignTrailing|AlignVCenter</set>
</property>
<property name="text">
<string>Car:</string>
</property>
</widget>
<widget class="KexiDBLabel">
<property name="name">
<string>label3</string>
</property>
<property name="geometry">
<rect>
<x>20</x>
<y>80</y>
<width>70</width>
<height>30</height>
</rect>
</property>
<property name="alignment">
<set>AlignTrailing|AlignVCenter</set>
</property>
<property name="text">
<string>Since:</string>
</property>
</widget>
<widget class="KexiDBLineEdit">
<property name="name">
<string>textBox</string>
</property>
<property name="geometry">
<rect>
<x>100</x>
<y>80</y>
<width>92</width>
<height>26</height>
</rect>
</property>
<property name="dataSource">
<string>since</string>
</property>
</widget>
<widget class="KexiDBLabel">
<property name="name">
<string>TextLabel1</string>
</property>
<property name="geometry">
<rect>
<x>20</x>
<y>20</y>
<width>306</width>
<height>54</height>
</rect>
</property>
<property name="alignment">
<set>AlignLeading|AlignTop</set>
</property>
<property name="dataSource">
<string></string>
</property>
<property name="text">
<string>Ownership</string>
</property>
<property name="font">
<font>
<family>Verdana</family>
<pointsize>20</pointsize>
<weight>50</weight>
<bold>0</bold>
<italic>0</italic>
<underline>0</underline>
<strikeout>0</strikeout>
</font>
</property>
</widget>
<widget class="KexiDBImageBox">
<property name="name">
<string>image</string>
</property>
<property name="geometry">
<rect>
<x>350</x>
<y>20</y>
<width>145</width>
<height>119</height>
</rect>
</property>
<property name="alignment">
<set>AlignTrailing|AlignTop</set>
</property>
<property name="storedPixmapId">
<number>2</number>
</property>
<property name="keepAspectRatio">
<bool>true</bool>
</property>
</widget>
</widget>
<layoutDefaults margin="11" spacing="6"/>
<tabstops>
<tabstop>image</tabstop>
<tabstop>textBox</tabstop>
<tabstop>comboBox</tabstop>
<tabstop>comboBox2</tabstop>
</tabstops>
</UI>
',NULL);
INSERT INTO "kexi__objectdata" VALUES(106,'<!DOCTYPE macros>
<macro xmlversion="1" >
  <item action="open" >
    <variable name="name" >cars</variable>
    <variable name="object" >form</variable>
  </item>
  <item action="navigate" >
    <variable name="record" >last</variable>
  </item>
</macro>
',NULL);
INSERT INTO "kexi__objectdata" VALUES(107,'SELECT name, surname, age FROM persons WHERE persons.age > [Enter minimum person''s age] ORDER BY age','sql');
INSERT INTO "kexi__objectdata" VALUES(107,NULL,'query_layout');
INSERT INTO "kexi__objectdata" VALUES(110,'<!DOCTYPE kexireport>
<kexireport>
 <report:content xmlns:report="http://kexi-project.org/report/2.0" xmlns:svg="urn:oasis:names:tc:opendocument:xmlns:svg-compatible:1.0" xmlns:fo="urn:oasis:names:tc:opendocument:xmlns:xsl-fo-compatible:1.0">
  <report:title>Report</report:title>
  <report:script report:script-interpreter="javascript"></report:script>
  <report:grid report:grid-divisions="4" report:grid-snap="1" report:grid-visible="1" report:page-unit="cm"/>
  <report:page-style report:print-orientation="portrait" fo:margin-right="28.346505829687491pt" report:page-size="A4" fo:margin-top="28.346505829687491pt" fo:margin-left="28.346505829687491pt" fo:margin-bottom="28.346505829687491pt">predefined</report:page-style>
  <report:body>
   <report:section fo:background-color="#ffffff" report:section-type="header-page-any" svg:height="92.126145876178711pt">
    <report:line svg:y2="85.039518379687138pt" svg:x1="0.000000000000000pt" svg:x2="538.583616404685813pt" report:name="line6" report:z-index="0" svg:y1="85.039519181249347pt">
     <report:line-style report:line-style="solid" report:line-color="#000000" report:line-weight="1"/>
    </report:line>
    <report:label svg:width="55.859411235962881pt" report:vertical-align="top" svg:x="481.890597270684339pt" svg:y="63.779639051993712pt" report:horizontal-align="left" report:name="label7" report:caption="Owner since" report:z-index="0" svg:height="17.007907042748343pt">
     <report:text-style fo:background-color="#ffffff" fo:font-family="Nokia Pure Text" fo:foreground-color="#000000" fo:background-opacity="0%" style:letter-kerning="true" fo:font-weight="bold" fo:font-size="10" fo:letter-spacing="0%"/>
     <report:line-style report:line-style="nopen" report:line-color="#000000" report:line-weight="1"/>
    </report:label>
    <report:label svg:width="113.386025812617220pt" report:vertical-align="top" svg:x="361.417954627964150pt" svg:y="63.779639051993712pt" report:horizontal-align="left" report:name="label6" report:caption="Car model&#xa;" report:z-index="0" svg:height="17.007904281600400pt">
     <report:text-style fo:background-color="#ffffff" fo:font-family="Nokia Pure Text" fo:foreground-color="#000000" fo:background-opacity="0%" style:letter-kerning="true" fo:font-weight="bold" fo:font-size="10" fo:letter-spacing="0%"/>
     <report:line-style report:line-style="nopen" report:line-color="#000000" report:line-weight="1"/>
    </report:label>
    <report:label svg:width="35.433134068441127pt" report:vertical-align="top" svg:x="318.898195259968531pt" svg:y="63.779639051993712pt" report:horizontal-align="left" report:name="label5" report:caption="Age" report:z-index="0" svg:height="17.007904103461833pt">
     <report:text-style fo:background-color="#ffffff" fo:font-family="Nokia Pure Text" fo:foreground-color="#000000" fo:background-opacity="0%" style:letter-kerning="true" fo:font-weight="bold" fo:font-size="10" fo:letter-spacing="0%"/>
     <report:line-style report:line-style="nopen" report:line-color="#000000" report:line-weight="1"/>
    </report:label>
    <report:label svg:width="162.992416885545339pt" report:vertical-align="top" svg:x="148.819157787985318pt" svg:y="63.779639051993712pt" report:horizontal-align="left" report:name="label4" report:caption="Surname" report:z-index="0" svg:height="17.007904103461833pt">
     <report:text-style fo:background-color="#ffffff" fo:font-family="Nokia Pure Text" fo:foreground-color="#000000" fo:background-opacity="0%" style:letter-kerning="true" fo:font-weight="bold" fo:font-size="10" fo:letter-spacing="0%"/>
     <report:line-style report:line-style="nopen" report:line-color="#000000" report:line-weight="1"/>
    </report:label>
    <report:label svg:width="141.732535605760205pt" report:vertical-align="top" svg:x="0.000000000000000pt" svg:y="63.779639051993712pt" report:horizontal-align="left" report:name="label3" report:caption="Name&#xa;" report:z-index="0" svg:height="17.007904156903383pt">
     <report:text-style fo:background-color="#ffffff" fo:font-family="Nokia Pure Text" fo:foreground-color="#000000" fo:background-opacity="0%" style:letter-kerning="true" fo:font-weight="bold" fo:font-size="10" fo:letter-spacing="0%"/>
     <report:line-style report:line-style="nopen" report:line-color="#000000" report:line-weight="1"/>
    </report:label>
    <report:label svg:width="538.583682969140341pt" report:vertical-align="top" svg:x="0.000000000000000pt" svg:y="0.000000000000000pt" report:horizontal-align="left" report:name="label2" report:caption="Persons &amp; Cars&#xa;" report:z-index="0" svg:height="49.606386344969096pt">
     <report:text-style fo:background-color="#ffffff" fo:font-family="Nokia Pure Text" fo:foreground-color="#000000" fo:background-opacity="0%" style:letter-kerning="true" fo:font-size="32" fo:letter-spacing="0%"/>
     <report:line-style report:line-style="nopen" report:line-color="#000000" report:line-weight="1"/>
    </report:label>
   </report:section>
   <report:detail>
    <report:section fo:background-color="#ffffff" report:section-type="detail" svg:height="21.259879839845279pt">
     <report:field report:word-wrap="0" report:value="" svg:width="55.859418705556280pt" report:vertical-align="center" report:can-grow="0" svg:x="481.890608189445572pt" svg:y="0.000000000000000pt" report:horizontal-align="left" report:name="field11" report:item-data-source="since" report:z-index="0" svg:height="14.173253508619686pt">
      <report:text-style fo:background-color="#ffffff" fo:font-family="Nokia Pure Text" fo:foreground-color="#000000" fo:background-opacity="0%" style:letter-kerning="true" fo:font-size="10" fo:letter-spacing="0%"/>
      <report:line-style report:line-style="nopen" report:line-color="#000000" report:line-weight="1"/>
     </report:field>
     <report:field report:word-wrap="0" report:value="" svg:width="113.386025575099126pt" report:vertical-align="center" report:can-grow="0" svg:x="361.417956142084051pt" svg:y="0.000000000000000pt" report:horizontal-align="left" report:name="field10" report:item-data-source="model" report:z-index="0" svg:height="14.173254280548935pt">
      <report:text-style fo:background-color="#ffffff" fo:font-family="Nokia Pure Text" fo:foreground-color="#000000" fo:background-opacity="0%" style:letter-kerning="true" fo:font-size="10" fo:letter-spacing="0%"/>
      <report:line-style report:line-style="nopen" report:line-color="#000000" report:line-weight="1"/>
     </report:field>
     <report:field report:word-wrap="0" report:value="" svg:width="35.433133697324813pt" report:vertical-align="center" report:can-grow="0" svg:x="318.898202607903556pt" svg:y="0.000000000000000pt" report:horizontal-align="left" report:name="field9" report:item-data-source="age" report:z-index="0" svg:height="14.173254132100123pt">
      <report:text-style fo:background-color="#ffffff" fo:font-family="Nokia Pure Text" fo:foreground-color="#000000" fo:background-opacity="0%" style:letter-kerning="true" fo:font-size="10" fo:letter-spacing="0%"/>
      <report:line-style report:line-style="nopen" report:line-color="#000000" report:line-weight="1"/>
     </report:field>
     <report:field report:word-wrap="0" report:value="" svg:width="162.992415007693978pt" report:vertical-align="center" report:can-grow="0" svg:x="148.819161217021616pt" svg:y="0.000000000000000pt" report:horizontal-align="left" report:name="field8" report:item-data-source="surname" report:z-index="0" svg:height="14.173255097017455pt">
      <report:text-style fo:background-color="#ffffff" fo:font-family="Nokia Pure Text" fo:foreground-color="#000000" fo:background-opacity="0%" style:letter-kerning="true" fo:font-size="10" fo:letter-spacing="0%"/>
      <report:line-style report:line-style="nopen" report:line-color="#000000" report:line-weight="1"/>
     </report:field>
     <report:field report:word-wrap="0" report:value="" svg:width="141.732534908061524pt" report:vertical-align="center" report:can-grow="0" svg:x="0.000000000000000pt" svg:y="0.000000000000000pt" report:horizontal-align="left" report:name="field1" report:item-data-source="name" report:z-index="0" svg:height="14.173253746134368pt">
      <report:text-style fo:background-color="#ffffff" fo:font-family="Nokia Pure Text" fo:foreground-color="#000000" fo:background-opacity="0%" style:letter-kerning="true" fo:font-size="10" fo:letter-spacing="0%"/>
      <report:line-style report:line-style="nopen" report:line-color="#000000" report:line-weight="1"/>
     </report:field>
    </report:section>
   </report:detail>
  </report:body>
 </report:content>
 <connection type="internal" class="org.kexi-project.query" source="persons_and_cars"/>
</kexireport>
','layout');
INSERT INTO "kexi__objectdata" VALUES(111,'<!DOCTYPE kexireport>
<kexireport>
 <report:content xmlns:report="http://kexi-project.org/report/2.0" xmlns:svg="urn:oasis:names:tc:opendocument:xmlns:svg-compatible:1.0" xmlns:fo="urn:oasis:names:tc:opendocument:xmlns:xsl-fo-compatible:1.0">
  <report:title>Report</report:title>
  <report:script report:script-interpreter="javascript"></report:script>
  <report:grid report:grid-divisions="4" report:grid-snap="1" report:grid-visible="1" report:page-unit="cm"/>
  <report:page-style report:print-orientation="portrait" fo:margin-right="28.346505829687491pt" report:page-size="A4" fo:margin-top="28.346505829687491pt" fo:margin-left="28.346505829687491pt" fo:margin-bottom="28.346505829687491pt">predefined</report:page-style>
  <report:body>
   <report:section fo:background-color="#ffffff" report:section-type="header-page-any" svg:height="92.126146165631766pt">
    <report:label svg:width="538.583686353643998pt" report:vertical-align="top" svg:x="0.000000000000000pt" svg:y="0.000000000000000pt" report:horizontal-align="left" report:name="label2" report:caption="Cars&#xa;" report:z-index="0" svg:height="49.606386656699641pt">
     <report:text-style fo:background-color="#ffffff" fo:font-family="Nokia Pure Text" fo:foreground-color="#000000" fo:background-opacity="0%" style:letter-kerning="true" fo:font-size="32" fo:letter-spacing="0%"/>
     <report:line-style report:line-style="nopen" report:line-color="#000000" report:line-weight="1"/>
    </report:label>
    <report:label svg:width="28.346505859379526pt" report:vertical-align="top" svg:x="0.000000000000000pt" svg:y="63.779638116801969pt" report:horizontal-align="left" report:name="label3" report:caption="#" report:z-index="0" svg:height="17.007904424107139pt">
     <report:text-style fo:background-color="#ffffff" fo:font-family="Nokia Pure Text" fo:foreground-color="#000000" fo:background-opacity="0%" style:letter-kerning="true" fo:font-weight="bold" fo:font-size="10" fo:letter-spacing="0%"/>
     <report:line-style report:line-style="nopen" report:line-color="#000000" report:line-weight="1"/>
    </report:label>
    <report:label svg:width="492.311668943977452pt" report:vertical-align="top" svg:x="35.433132287112194pt" svg:y="63.779638116801969pt" report:horizontal-align="left" report:name="label4" report:caption="Model" report:z-index="0" svg:height="17.007904602245731pt">
     <report:text-style fo:background-color="#ffffff" fo:font-family="Nokia Pure Text" fo:foreground-color="#000000" fo:background-opacity="0%" style:letter-kerning="true" fo:font-weight="bold" fo:font-size="10" fo:letter-spacing="0%"/>
     <report:line-style report:line-style="nopen" report:line-color="#000000" report:line-weight="1"/>
    </report:label>
    <report:line svg:y2="85.039518646874541pt" svg:x1="0.000000000000000pt" svg:x2="538.583618096872897pt" report:name="line0" report:z-index="0" svg:y1="85.039519448436749pt">
     <report:line-style report:line-style="solid" report:line-color="#000000" report:line-weight="1"/>
    </report:line>
   </report:section>
   <report:detail>
    <report:section fo:background-color="#ffffff" report:section-type="detail" svg:height="21.259879906642130pt">
     <report:field report:word-wrap="0" report:value="" svg:width="28.346505829689765pt" report:vertical-align="center" report:can-grow="0" svg:x="0.000000000000000pt" svg:y="0.000000000000000pt" report:horizontal-align="left" report:name="field1" report:item-data-source="id" report:z-index="0" svg:height="14.173253835200253pt">
      <report:text-style fo:background-color="#ffffff" fo:font-family="Nokia Pure Text" fo:foreground-color="#000000" fo:background-opacity="0%" style:letter-kerning="true" fo:font-size="10" fo:letter-spacing="0%"/>
      <report:line-style report:line-style="nopen" report:line-color="#000000" report:line-weight="1"/>
     </report:field>
     <report:field report:word-wrap="0" report:value="" svg:width="489.810838841154009pt" report:vertical-align="center" report:can-grow="0" svg:x="35.433132287112194pt" svg:y="0.000000000000000pt" report:horizontal-align="left" report:name="field8" report:item-data-source="model" report:z-index="0" svg:height="14.173255482980993pt">
      <report:text-style fo:background-color="#ffffff" fo:font-family="Nokia Pure Text" fo:foreground-color="#000000" fo:background-opacity="0%" style:letter-kerning="true" fo:font-size="10" fo:letter-spacing="0%"/>
      <report:line-style report:line-style="nopen" report:line-color="#000000" report:line-weight="1"/>
     </report:field>
    </report:section>
   </report:detail>
  </report:body>
 </report:content>
 <connection type="internal" class="org.kexi-project.table" source="cars"/>
</kexireport>
','layout');
INSERT INTO "kexi__objectdata" VALUES(112,'<!DOCTYPE kexireport>
<kexireport>
 <report:content xmlns:report="http://kexi-project.org/report/2.0" xmlns:svg="urn:oasis:names:tc:opendocument:xmlns:svg-compatible:1.0" xmlns:fo="urn:oasis:names:tc:opendocument:xmlns:xsl-fo-compatible:1.0">
  <report:title>Report</report:title>
  <report:script report:script-interpreter="javascript"></report:script>
  <report:grid report:grid-divisions="4" report:grid-snap="1" report:grid-visible="1" report:page-unit="cm"/>
  <report:page-style report:print-orientation="portrait" fo:margin-right="28.346505829687491pt" report:page-size="A4" fo:margin-top="28.346505829687491pt" fo:margin-left="28.346505829687491pt" fo:margin-bottom="28.346505829687491pt">predefined</report:page-style>
  <report:body>
   <report:section fo:background-color="#ffffff" report:section-type="header-page-any" svg:height="92.126147130475275pt">
    <report:label svg:width="538.583697635323801pt" report:vertical-align="top" svg:x="0.000000000000000pt" svg:y="0.000000000000000pt" report:horizontal-align="left" report:name="label2" report:caption="Persons" report:z-index="0" svg:height="49.606387695801459pt">
     <report:text-style fo:background-color="#ffffff" fo:font-family="Nokia Pure Text" fo:foreground-color="#000000" fo:background-opacity="0%" style:letter-kerning="true" fo:font-size="32" fo:letter-spacing="0%"/>
     <report:line-style report:line-style="nopen" report:line-color="#000000" report:line-weight="1"/>
    </report:label>
    <report:label svg:width="28.346506453152038pt" report:vertical-align="top" svg:x="0.000000000000000pt" svg:y="63.779639452790200pt" report:horizontal-align="left" report:name="label3" report:caption="#" report:z-index="0" svg:height="17.007904780370644pt">
     <report:text-style fo:background-color="#ffffff" fo:font-family="Nokia Pure Text" fo:foreground-color="#000000" fo:background-opacity="0%" style:letter-kerning="true" fo:font-weight="bold" fo:font-size="10" fo:letter-spacing="0%"/>
     <report:line-style report:line-style="nopen" report:line-color="#000000" report:line-weight="1"/>
    </report:label>
    <report:label svg:width="134.645905370430285pt" report:vertical-align="top" svg:x="35.433132955106288pt" svg:y="63.779639319191375pt" report:horizontal-align="left" report:name="label4" report:caption="Name" report:z-index="0" svg:height="17.007905190089406pt">
     <report:text-style fo:background-color="#ffffff" fo:font-family="Nokia Pure Text" fo:foreground-color="#000000" fo:background-opacity="0%" style:letter-kerning="true" fo:font-weight="bold" fo:font-size="10" fo:letter-spacing="0%"/>
     <report:line-style report:line-style="nopen" report:line-color="#000000" report:line-weight="1"/>
    </report:label>
    <report:line svg:y2="85.039519537499217pt" svg:x1="0.000000000000000pt" svg:x2="538.583623737496396pt" report:name="line2" report:z-index="0" svg:y1="85.039520339061426pt">
     <report:line-style report:line-style="solid" report:line-color="#808080" report:line-weight="1"/>
    </report:line>
    <report:label svg:width="162.992411764205002pt" report:vertical-align="top" svg:x="177.165664775531638pt" svg:y="63.779639319191375pt" report:horizontal-align="left" report:name="label6" report:caption="Surname" report:z-index="0" svg:height="17.007905920456210pt">
     <report:text-style fo:background-color="#ffffff" fo:font-family="Nokia Pure Text" fo:foreground-color="#000000" fo:background-opacity="0%" style:letter-kerning="true" fo:font-weight="bold" fo:font-size="10" fo:letter-spacing="0%"/>
     <report:line-style report:line-style="nopen" report:line-color="#000000" report:line-weight="1"/>
    </report:label>
    <report:label svg:width="42.519759590662197pt" report:vertical-align="top" svg:x="347.244702960041479pt" svg:y="63.779639319191375pt" report:horizontal-align="left" report:name="label7" report:caption="Age" report:z-index="0" svg:height="17.007906347988826pt">
     <report:text-style fo:background-color="#ffffff" fo:font-family="Nokia Pure Text" fo:foreground-color="#000000" fo:background-opacity="0%" style:letter-kerning="true" fo:font-weight="bold" fo:font-size="10" fo:letter-spacing="0%"/>
     <report:line-style report:line-style="nopen" report:line-color="#000000" report:line-weight="1"/>
    </report:label>
   </report:section>
   <report:detail>
    <report:section fo:background-color="#ffffff" report:section-type="detail" svg:height="21.259879505861022pt">
     <report:line svg:y2="17.007903586874971pt" svg:x1="0.000000000000000pt" svg:x2="538.583624301558757pt" report:name="line6" report:z-index="0" svg:y1="17.007903586874971pt">
      <report:line-style report:line-style="solid" report:line-color="#808080" report:line-weight="0.5"/>
     </report:line>
     <report:field report:word-wrap="0" report:value="" svg:width="28.346506423462266pt" report:vertical-align="center" report:can-grow="0" svg:x="0.000000000000000pt" svg:y="0.000000000000000pt" report:horizontal-align="left" report:name="field1" report:item-data-source="id" report:z-index="0" svg:height="14.173254132086521pt">
      <report:text-style fo:background-color="#ffffff" fo:font-family="Nokia Pure Text" fo:foreground-color="#000000" fo:background-opacity="0%" style:letter-kerning="true" fo:font-size="10" fo:letter-spacing="0%"/>
      <report:line-style report:line-style="nopen" report:line-color="#000000" report:line-weight="1"/>
     </report:field>
     <report:field report:word-wrap="0" report:value="" svg:width="134.645905652482980pt" report:vertical-align="center" report:can-grow="0" svg:x="35.433132287112194pt" svg:y="0.000000000000000pt" report:horizontal-align="left" report:name="field8" report:item-data-source="name" report:z-index="0" svg:height="14.173255809557070pt">
      <report:text-style fo:background-color="#ffffff" fo:font-family="Nokia Pure Text" fo:foreground-color="#000000" fo:background-opacity="0%" style:letter-kerning="true" fo:font-size="10" fo:letter-spacing="0%"/>
      <report:line-style report:line-style="nopen" report:line-color="#000000" report:line-weight="1"/>
     </report:field>
     <report:field report:word-wrap="0" report:value="" svg:width="162.992411934921108pt" report:vertical-align="center" report:can-grow="0" svg:x="177.165661435561020pt" svg:y="0.000000000000000pt" report:horizontal-align="left" report:name="field9" report:item-data-source="surname" report:z-index="0" svg:height="14.173255972849645pt">
      <report:text-style fo:background-color="#ffffff" fo:font-family="Nokia Pure Text" fo:foreground-color="#000000" fo:background-opacity="0%" style:letter-kerning="true" fo:font-size="10" fo:letter-spacing="0%"/>
      <report:line-style report:line-style="nopen" report:line-color="#000000" report:line-weight="1"/>
     </report:field>
     <report:field report:word-wrap="0" report:value="" svg:width="42.519759635196834pt" report:vertical-align="center" report:can-grow="0" svg:x="347.244696413699558pt" svg:y="0.000000000000000pt" report:horizontal-align="left" report:name="field10" report:item-data-source="age" report:z-index="0" svg:height="14.173256403350127pt">
      <report:text-style fo:background-color="#ffffff" fo:font-family="Nokia Pure Text" fo:foreground-color="#000000" fo:background-opacity="0%" style:letter-kerning="true" fo:font-size="10" fo:letter-spacing="0%"/>
      <report:line-style report:line-style="nopen" report:line-color="#000000" report:line-weight="1"/>
     </report:field>
    </report:section>
   </report:detail>
  </report:body>
 </report:content>
 <connection type="internal" class="org.kexi-project.table" source="persons"/>
</kexireport>
','layout');
CREATE TABLE kexi__objects (o_id INTEGER PRIMARY KEY, o_type UNSIGNED Byte, o_name Text(200), o_caption Text(200), o_desc CLOB);
INSERT INTO "kexi__objects" VALUES(1,1,'persons','Persons in our company',NULL);
INSERT INTO "kexi__objects" VALUES(2,1,'cars','Cars',NULL);
INSERT INTO "kexi__objects" VALUES(4,3,'persons','Persons in our company',NULL);
INSERT INTO "kexi__objects" VALUES(65,3,'cars','Cars',NULL);
INSERT INTO "kexi__objects" VALUES(103,1,'ownership','Ownership',NULL);
INSERT INTO "kexi__objects" VALUES(104,2,'persons_and_cars','Persons and cars',NULL);
INSERT INTO "kexi__objects" VALUES(105,3,'ownership','Ownership',NULL);
INSERT INTO "kexi__objects" VALUES(107,2,'persons_by_age','Persons by age',NULL);
INSERT INTO "kexi__objects" VALUES(110,4,'persons_and_cars','Persons and cars',NULL);
INSERT INTO "kexi__objects" VALUES(111,4,'cars','Cars',NULL);
INSERT INTO "kexi__objects" VALUES(112,4,'persons','Persons',NULL);
CREATE TABLE kexi__parts (p_id INTEGER PRIMARY KEY, p_name Text(200), p_mime Text(200), p_url Text(200));
INSERT INTO "kexi__parts" VALUES(1,'Tables','kexi/table','org.kexi-project.table');
INSERT INTO "kexi__parts" VALUES(2,'Queries','kexi/query','org.kexi-project.query');
INSERT INTO "kexi__parts" VALUES(3,'Forms','kexi/form','org.kexi-project.form');
INSERT INTO "kexi__parts" VALUES(4,'Reports','kexi/report','org.kexi-project.report');
INSERT INTO "kexi__parts" VALUES(5,'Scripts','kexi/script','org.kexi-project.script');
INSERT INTO "kexi__parts" VALUES(7,'Macros','kexi/macro','org.kexi-project.macro');
CREATE TABLE kexi__querydata (q_id UNSIGNED Integer, q_sql CLOB, q_valid Boolean);
CREATE TABLE kexi__queryfields (q_id UNSIGNED Integer, f_order Integer, f_id Integer, f_tab_asterisk UNSIGNED Integer, f_alltab_asterisk Boolean);
CREATE TABLE kexi__querytables (q_id UNSIGNED Integer, t_id UNSIGNED Integer, t_order UNSIGNED Integer);
CREATE TABLE persons (id INTEGER PRIMARY KEY, age UNSIGNED Integer, name Text(200), surname Text(200));
INSERT INTO "persons" VALUES(1,32,'Georege','Foster');
INSERT INTO "persons" VALUES(2,62,'Joan','Shelley');
INSERT INTO "persons" VALUES(3,51,'William','Gates ™');
INSERT INTO "persons" VALUES(4,45,'John','Smith');
INSERT INTO "persons" VALUES(10,58,'Александр','Пушкин');
INSERT INTO "persons" VALUES(12,79,'Μιχαήλ','Στεφανόπουλος');
CREATE TABLE kexi__blobs (o_id INTEGER PRIMARY KEY, o_data BLOB, o_name Text(200), o_caption Text(200), o_mime Text(200) NOT NULL, o_folder_id Integer UNSIGNED);
INSERT INTO "kexi__blobs" VALUES(1,X'89504E470D0A1A0A0000000D4948445200000040000000400806000000AA6971DE0000000467414D410000D6D8D44F58320000001974455874536F6674776172650041646F626520496D616765526561647971C9653C0000198D4944415478DAED5B678C1CE7797EA76E2FB77BFDC863BBA34452128F9DB444CA8A154782E5A6207612C5254E641836D224FFB09318F9E3048813FF082040708240361077C5708B1D4B242536B1533CF2588EE4B5BDB6B777B77BDB7767A7E4F9DE9DD5DE99B10C1B8243C71CEAE5ECCC4EF99EB73C6F594A721C877E93371972570177157057017715F01BBBA9FFF0E2303D7560351FD42C9B4452302D876C7C68EC2DBB2EB64DD8DB1082B8DF5910EC6DBE86F0D916D7F17973C53584F37C2F6F12A46ADAFC4ED9D4C9512C722408BE51F0A54F5768F926416C48BE64625D58A3842B6D12CFF7ABB2D4AF6BF2DE96A03ED013F3EEF3EA72F7F5A9C2D7B265F3594596AC375500DDF91B1B45A4EB1A145AADD92DB22C6DF569CAAEB690BEBDB3C5B3A7BBC5DBD31AD6F59057257C47BA2A537BD8F317DF3F9BFC366E3941D2AF990280B5E1758A24510700EDF0EBEAAEF6887707ACBB331EF2B4B54774C9EF51D863F21593021E99E6F306DD9C29D2FA4E3FF5B6F908DFBFBD54354FC824DDB90A709601361DDB2791BCDEA7CB7BE221FD81F6886737F6F775443DA1684023AF26132EA3B221420982CF13A9122516CAB46D7D9826172B54A85A846B99DCD6B5FB1E140AD1D43B4801ECCEE28FCD1C10C6F1FD9AE6EC8E05F5EDED91E09EB688DEBBBAD5E789857476E5C63DCC49405F03EA4AADAE004596091EC1C713F3159A4D57696377804CF04A16FC82E7EC4ACC97E350C0E2FF990278F136C7AF6C5B769B2CCBDB11BF3BE3617D7B4B40DF1DF26B9D706739E453616105A0602D89EF61208D4ADDDDF1F7E29289C52AA572558A057492707C73B6C81E22B6D94C955AE005506A2BC0EF81AE7E2449BF1A0534015BB607BB5578F19E904FDB110D6A3B6321ED019053146C4D3E8F0B165BCDB4015A660B174B25F27A7482A25684C9F2035992004EA3E45295227E95E6730A654B16F57505E88BDF1FA3CE884EBFBFBF8B3674F829ECD30E640A061420BDF50A6802E69419C0EE1E80DA1BF0A803119FBA2F1AD4D7C395FD619FCA69ED7FE322C374186C2AB540B33333E4F3EAB476DD3A9215101CC7CB0AEC6C6D19826C8010A92B429061088AF0207F6E5A15A457AE2C52E8BC4A9F7ABC97DAA39EFD0BB92A5E21D96F850218B05B1BB4E270ABA248BB833E7D7BC0A3EC8E04B42E585A0B78B1188EDFDB37DB8158166596B2945E5CA472A54AE148144AB4C8340D8AB57651095E10D175A295F81B075C770CCF16A004874A8645EB3B7C94CA1A746DBA481BBBFC386FD1FE4D31AA1A0E21550E007C2F118DFF520A70016B906E2C7E8FAA483B025E6527D2D256B8763CE467EBBE19D332E86AB54A1680D76A35CAE5720C52D3345AD7B79EAE0E5DA1B6B60ECA66B3948562BA7B569144122B9A2467050FE09095BB6D6D9816F235BA0196BFB72748AD210E09B67EB16CD29EBE2855A088167820AE7FC8B09C7159FA790A68A6253F5EDE87FD5E001EF06AEA5EAFAE6C0C7A9580DFAB32D9A8CACFACA2D93DA7D3153073991E581322A432326B268D8C8E900ECBAE5BB71E60976871314DE1809F5A62AD542E9729128D92CFE7A3999969EAECEC22AFD7CBE09B9ED0DC07BD2AE2DE64251D01E863D7329429D6E84F1F5DCDA123C2AE889488E248A4C58792D9EA7F803B6E5780FB501D77EDC5C3F640F3DB3C282B3D9ADA8314A30BEB023C13D6CFDA4A78D1D442854653451A9B2BD13872F312169387253EF1D85A3AB0394E7E002D158B94CFE7A8BBBB9B02A1088D8D4FD04C728132E945EA5DBB817CA5002D2D65A8B5AD9D3428CABE9D037893DC4F61BF4615C326F00DA7C72B93798A81FD5116C3DB6C41AE9C2EDB229E03B3998A0EF2306E5300706D85FCBB4753B645344D462AE207FC2CC08E5B8F8FCF97045896043EA31A1396E7FBF02CD21499F338BE170AE0F3B1589CEBF9B1B109F2F903140C85696E2EC9E003F81C0517783CABB87740CA6C589E45AA7B261F21A4980035A59E3215ECEF87A76D591D74B34AF3DE72CD4678E8FDB866130E076F530058FA79E4CB1D4168F1F6AD5EA16561CD11615988B0F2240017CA35F1700E051F00C36BC0C61A69B2CC0B8232F8DC58AAC41EA2421991963899E081DE75ADEC0D5E9F1FAC2E0B2B313F38D8E37885B91DF7C30A1E60D00A2DE6CA647216E10B60E026E7E023F348A9021E086A2A3C793F2AC8C19FB6AB8A936B007205E9CD814C469245B6F2284027D3150661980022B35AC0B01605548B74095697742CDE43BAC2FC50D77CD5610F582AD4687AB1C2B57948B0BDC3D6457CFBB83B843048976C792FDD069C5133209C6065E6F2506C3E83B2187C108EE2B40D698447F39E9A5010F62898F64F56CACFC98AB45201E373C533D952ED7D8827713D5BFC4B2F8D735321C9CCB8EC6A7E0F2CAC398839836AD532BDADDBA6BD6B14928D259A4FE7A818BA87AEA42CB29510C09B752BC13446CD6245AE6B477AB22D976459DCCF2E7871E4D6141C7E8D2C2040E1619669216596C9EFF5D0C4CC1CD70D45A4D0AAE3A560A4854B6B8941AF0C58D32DA1C1030FA2670889085EA1009493479399CAFBD052B26655805D870A6A0A56ABB797AC55B65415716E558BF4E466A2776C0E9325A9586484366C50A8904D9377E13A5D32B7D17CC5C79E807BF97978B1F09E26681B229EF986321825BBAE61C162664D200121A6913D6AFCEE4C264381809F5D3F932B512018A6484CA7744926DBB45CE04DBEB0EB7BFEBB0883200CBA91AAB7E3FB23CB752417CBB563E3A9A2652E0B837BBA838494C715163A3306A3C936172D039D16FDD6E60819A4912DA92CE9A51C158A65341F5EEA285C20AB66F052148909114D0A7769F5F864F042A4A67B4353954A856A009E981847AD6022154E511EF5C2126A83B9D41CF7F9ED1D9D246B7EEAE8ECA1CE9ED5D8778158A364D573A51B4A107253A7CD82E7725A94FCBA72005FAF0C01DC30343A571C0559F47BDD29CC86CE00A7184DE1C683AB34DB04A9218B6CEF456E96141436261452A06C264D2DF118F5DF7B2F99E52E5A9C7F89AE2CA4104ABD00EF30A9813FB848598F3030393D996C5559564461C44552B158A04221C7FC50AD999C31D2E934ADEEECA61C6A06BFDF87D5FA61759FCB74EC95BC46A7497CBC0193F8CC242CC83D08436AF06E10FEFE6CB948F2B21A46858B56C6E60AC77225B31F652C6B3A863D464B5C58E88AC439D58451FDBA43F1A00A66ADB26BB5B5775067770FB3B96D995CD3AF5FDB43DDB90C8D5B3D00AF719E45DBCB21B51EA16500702A39438AAA3178D405AC085485488D417C56B9BD6D8F47513B84B827080583942B1AB8DEC2339B3332D9AE839497F10AB071A3058B134A400E5FFC6143A220DB056FEF20F0FC1B0A100F4CE78DE353E9D2C77AE0C25000CB9A761F15264C7E98050096A1501900D3258756B77928D0D24E8EAC0966765F5E9FF7B575ADA687AA199A1D154A6285B2F6C133ECFE1E5DE36A5051113EA2F68F465103844852BD38E7111EC3F19B2B5541BAE0856A05C5D21C9E8DD150570FBF47A26675283C4081803CB9218A043C083D1B6BAEC1A3CAF09C005F6F92231410C55AF6611DDF6D2852C579B8A8796C2C59A8ECEC8B79355733BDAD7EBA851EDB27AA2A58C80121E6350FA5F2360078B0903A533B583067277800107005E7F779C8C642819D455764A16432A0C800DE100E0501C8A2AECE0D6C1D55115E65D0E252010B3711FB590E93684047CF50A40294110A474882011C3CC3E1D537ABC25840852708855669616E920AF90299E0A152B9449BEFDB8AF2DACFEB41F90C62D7F6674B061420B91C8007E060F4E66C7EA86C983B3D5A7DC0B02AEE25BF5B06DB36ECEC58140D7991CA4A9C9E700D8397640D8B2A9383EF65B788194A22FF3A8A707FCE02F0298481432847A9BF4B019828C77C2693E67A2027001B551E75CD2F2CA01AF4E2BD1A154D15E281E5DB280C05D4AA553775DA08191D1C81321A22F27C2BC2B198CF73598D43648910A5F1D96C641F88CE3CA01D98CE5414ACCB6205083032308EA58A4773A5DACE08D703128996163339EEA8540D60249CB77D183ED4E856AA42F787E0666C7A9B6C68DBB5098FD2934595ADCAEEE94E70C09994640504417A655A5C98C722837463F82AC078D84AB54A89622D712CD6242D10A3424D274F042C8FEF0BF084A585A4A81AD94B1445E6EAB158C8238394C983FB3DF0CC58BC8DE26DEDF8CE84227DFC5C376CB8754628DD8F2AB5BF6649D7EB1E60738B898ACD38866CF0CC6AB83E365E348A079A4997391D8A04A1C9E2BC4327C78A140C60FABADACF459103D7AB573126198641154B66CF5115EE299827C0472213709DAEE91E26B742218FD4D685CF1A056131902893217A7B54A3E00C0BC40720008EF71461AD320F4CC81135093C32DE4A417846B425CEF7AAAA0AD1709FC5CF096038C22DB86573FF922D5BA46B12A245DA8FD3D72589BBC1BA3B57AAD6C99BD3B9CCFE4D6D2D8D691466EE584C95C13840EFD1246E42B2007669DAA0AE589EE45A4170AC9B704B1CB39A1A22C5212640B8273F1F5C0D263729573639165B5A5A392ED114892CC00B0595513697A7C4E42287886D2345228EBD81105BBD241A2E2877EDFA7EE6129F4763D06C61B70486E5DD4992046E33A850B1380D9B36E3E4B923B2C37E78FBBF616D1C028D61E3DCB5A9DC3910E26F071595171E0FF264966FE2F242E3CF1099B2850ABD76798AB67554D885F1529EE8845ADAA8272ED1EC540DB584CE9ED45002C65F08218353940EF7AC1A655A4091C3EA91713F404E4C276909842989AED21FA63044915528212CC0719628180087D8568A622C66B83CC39D21AFA3C6B546E397297E7FA3AFE08C81EEF0411C78C15115B9C110302ABABCE25194C638E607B1E5315DE5C5FB0470F871002E188BF858EBCFFDD70815284C922740B61620C717A740AC13AD29C2C82C513A0B52CA16202566720305CE5CA6EC7285C923B1F9F914E5961650ECE4686636457A204EBA5794BC606CA4DA60B483FCE11814A2808B3C5C3F38C43D0367953C2C9C41C3952E1842B876295658010C4D6E2027F67528D483B961640DC06FE11050BCC1FA978865A4876337A6724E5F5750B2EB4AC10D2A938747971B9503BBECF54486C6667394810E7B7B3BB97CC5F338FE3674C5E8A987C5C8AA0A4FE1F4C78429943B91CCD3AEFE1628959F03220C901F56362C853AA35DB450B028A285D9A5011A404D37DD49B78DD91AA7964F7C57F60492F852547BE288D3E2D4C41452A1A2F83CA103F0F6F38A55AB527AEC22055A5711E9A174D42F7F64577F3C2C1EDA7829AA446E8C149CD3E109F34B25FACE91613271EF930FDF4B51BFCA31CCCD0CBAB38285148A185DD311A2BE5511EAEB0963641DA636BF040F00D34703E0018D53A017E0C1D7A4EA1A328E8398653767E1FA7E3930A709CCE741D8349B9E266849720951E3745D2A1468627C84AE0D5D64999F1DA7DE9E7692BCF152B6687C53BDF9EA57F8E6D1D7BE459B9EF874FE46D77B4E142BE60730F0E4161503D0463527DC9EBDE295F313A483E5FEF6A30F23FF76D39255E3F392E00E4804D7E26201822198A68D5465D09AEE282BB052AE90D2E24397463CB0700D05B2AA71E8AD44D4A8EF9BFB9610C2C3A7B0C2B92073BD2953B238E472D9454AA2994A23D5964A08D25004A9B193EED93240BDA82944A61B9AC8BC8D88A22AB99B51CAD2E0B7FF0EA4A21C9F7DF7BD1F88C05A500093A05F9745CCF1F477686C1E29A9429FFBD03EEA6E0B71DEF7F9FDDCFB2FEB46DCE186FD86F26C5DE529527B5B84E0DB625EC8E2FE10C4318BFC0C2C7C7FC3D2B78DC6D1A700BCCA55A1AE68F0BC1A5B39393D4EA39831265239D132736ADCD07F2FC5DB3B913AFDEC1535F65287FB114CC13A409EBB1A0A905830931ABB7CFCE4B5A9A7CD4DDDADF0EB3A6B7208915DAFE581F3A38F6F41DD5DAFDB03B0447D13809B7BDB252ADBAA7B02381BB1AE8A05F0B4165E46AADCECE2CAE019FC473CE66B58BDE9F3ACB4D6B04F4C78B9F0594ACFD1546204A5EF2C4ADF2C852311EA468BDC7FDF2E52BC518E7B94D36204C7844B16BFABF16F1B58897E8CC91AAB571B525A9C2DF6ED79D7930F0FAC6E951810577BDC8D212C78011878227E7576E7C622DDB11AB4EB3002545C6C25CB915C576585C0CD6DA4BF7AD9CC4EC397B3AD79B26B4158919C3A65C1FA9CD33D54A57C6A82AE5E3A4743174FD3F4E408DEAFD3FABE8DB4EB6D0FD37D037B28D0B68E477847919E11759CC156FCBAE496C3086B2ED05093480D0F90211EDE5BA5F2F9C1A1D3F9F2BE7B223EEEF41868BE582194057C23CA507629F75F8134C88717AAAA0C98C1D92E7289F82B00B7F99CAA365C1B209D7AA8F91022DD10440988D046785450E66661E16932B233E49497C81708C2A5BB68E3A6FBA9137302E44B5AC89974FCEAA879E2DC4BF3678E1F9EB831743E512995EC8E5DBF7BE0EF9F7DBA1BBF1772F5290903B8421031FD0A07BC9B1B0AB0210AC42B967DFDFC91F3A3F31FFAF08E355E0190ABAEC9B90C554B5EEAEB6D65ABE299FC304D55780FFC0C8E3137598B8B10CED9359B4FA9DCBA8A2EB25180C95C1663E6C093E26C6A06D61DA7B985341A2F85DAE22DB47E633FB576AC4208855040E9940371BEF8EAA5F2F153A712E74EBC3A3979E3C2B8B3349E154EE4E2B0E64ED9BEA9850FBEB7BF3B84F77286E277F9750D9C00C5A188BA3E3AF572430196AB040D124E8E5E1A1CBC3155D9B1A6CFDB20E3ADFD5DECB6A1905F581BD29CE7B9BCC5A4D53CE0CF8224B90EC0C6E0D9B5A17DA02603649ACDA601780215E12CBAC20CF94158B1D60EDAB0F15E0A45DBB8CAC426629995562C95EDCFFECDE78EFEE81B2F0C92952BBBEBF6B8E28754F1127DE7238FAFDBB2B68D170FBEE25F9C4BF0C0E1E905FACA0FAF954E1E3B7C70E8D56F3EA32E4B38861B0ADEEAECD5F973976F5DFFF0A37D03926BC1B55D51C6C5BFD9BB181B9BD4C8D1025F737ECF210261564677C8338072A94CF3A924CD4C275009CE8B3698197B55EF1AB4BD0F21A38470BDC6806D5B746FA63BF145E8E07C32992CFFF8ABFF72D67D85CF5D33377B909935F70CF8FEFCAFFFF1E38F3DFAC83A41DA8B28C68646A7E9E4B98B8BE78E1FBE71F5D299D7CDD92B27E12C878928A95273335C0963E99533278F0CA63FFED8405B00470C1CB1DEACC918A96B68EC5903EE79D6089F535419000C1E704C03F03C183BBB9411FD3EACDC4603DB77F35ED77D7C0FAA496E906CDC23D9DC91F25F8E78874BC69D1D1DDEF7FCDE1F6DF9C90FBE730DB3BD2A0CB4502A1646E189A344CA6CD7239FFAF2DACDBBD6BD7CEAB2FDDAE9B3D3678E1D1A9EB876EE756769EC02110D4366204BEC29BCDC955B0BA417E2B4DEFFF83BBFF58DAFFDD3239BA30C9EADFA26BFD830632BF534932F082BA760E5495A8495AB15F1C3670B4F757B7A7AF967304952D9C2887D06ED38CDE7E3881C0BC28A6C4E7A71394920E02A08657A2A318D2189C7B0A5C93FF98327DE317C65308B6F35EA1C785AD57CDBCCF99B09AACC5F21A29B902424C7A07FCEAFC345D70B628BB7CEDFBA70752CF7C8E66D616A6E8D18777980871D0CBA08024B2412FCCBEE5226CDFD781CC389AD5BB751675737185767865ACCC3F980C4A75A62DF24CAC68F9E1C46F8A440B8A660D2C4BB1C06AF8A2169C0277B83F7ADBE726BC679F1EB5FFE7A62EC56D9358B4CC9D77F6C12BDE45A9989F117F97782355753ED4E39553A75E1F2B0F1E4B65DAA5CE701ECDCC683DD558006D80CA560EDE45C52D4F63C29DEB66D3BB5B5C6C4F5ACF21B1359FBC47F9F5D3C78F0E0C8D98B4399F6BE3D9BBEF8D98FACDDB13E4615C3620BCBACD83A68F723F6F5BA5E87324D92B895BE7879CC3976EA7CEAF8D143B7AE9C7DE5557371F4072E0EC3B5F0D86D287F010538AED60C883E78FAE8F599A5A776AD8D29EC8E8651E57FDC908760F0C18A50359DDA3A3A919BB7A0E60E7075B784651CB998308E9EBA3079E8E59F8C5DBA706A243B71795E708BF0E4C4A9EF5FFBEA03FD7FBCF799F7B61835901D43758B5F497087C23FA8942D04EC7C914E0D5E318F1C3F3175E2C8A1D1B12B672F3BF9E9AB6E3C275CF72ED32FB9A974FB56826420B19991A19B8323296755282E8D4D2410CB55109686797D902DEDF17A798EDF609272D572BEF0FC572FBFF89DEFDDB879F9F484959B16CA34219A9BA622752F33AC81B511ABE1519A0CD154085101CABB3595A6E3E7864A875F792571FAC4D15B73C3E786C8CC8919DE08641AB20829700A74B7B75201366401B2B698189C3B3B7833FDC4B6D6783C86610780AB5A7320C1E454FFC1938727CFFDEBF3273EFFEC9F7DCF7DAE479C763F2B2E7BE4F6EEDB17FFAB4F7FE6D1F7BFEFEDAD8A0C8DA0B9C1F88F2E0ECFD0919317D2870EBE34F1FA991337721397AE4177375C979E7141976F03FDD62B80B72C7B8153E978EDF49991DAC70EC463B116CEE9B6CD8CDDE4C365A38A5C66A1EA82F52F0B29D35DFCF8838F7D70E72B3FFCDA27D14B280B88A033976E9A878F9D4C224C12575F3F356C2C8E0AC0B72013905948DA05EDD0B2ED57A1809A1B5BABAF5F7C6DE2D66C79F77D3D3EB25DE4CD01CCCAA2E7939FFCD46EFC8A5B9A9D9D5D5CD5D3132A148B89175E78E145CCFA2625C5335FEC3CB0F5335F7AE56A72EC4AF6CCF143A95B8327A7A83C9F705D7BB299A301FA0ED8FC9077AAB1BE7F7EE1E561CBC15643416042C4DEA8610F11FBAA10C342EA5EB98D8C4FFEA7DFEFF7BCA135D527E3AFCD387A37E4772003900E0E953B7093205B88E4673FF185EFCE0940707F564043A000ECED15A0F386E3A01DB59EF9FCF3A73BD76CFC38111D60F26B6E1E4818A2DF0920D537F9CEA9BBA43D75FAE489A982F9DEF6A0EACEF8E5953190424BFADAC51BC6E1578F4D1F3AF4726278F0D4B0959DBEE93275E6A75CBAEA0ADDD90A6892E1F8ADCBA7475FBDB4B8FD89ED71019E35831F53E9E899A1C24F0E1E4C9C38766464F2CA9961AAE5126E2CA796498EEEE04DA29FBFF590A2FFE17DEFFACBA79FFAE0FBD7CA9574E6F0E18363674F1EBB961EBD285255234DA55CD62EDC66E15F7305E8903D90ED44A239AFCD09AF80249701AEDC96AAFEDF28A0495C211764A909F8D77FBBFBFF0E13DDFD1F27EF2AE03779FB1F7564272FA5F86AD80000000049454E44AE426082','office-address-book.png','office-address-book','image/png',0);
INSERT INTO "kexi__blobs" VALUES(2,X'89504E470D0A1A0A0000000D4948445200000040000000400806000000AA6971DE000000017352474200AECE1CE900000006624B474400FF00FF00FFA0BDA793000000097048597300006EBA00006EBA01D6DEB1170000000774494D4507DB040909331633CB44E800000E554944415478DAED5B4DA8255711FEAA4EDF37BE971F8812132793C4F187E0307115B274639080F1077423D90AD9BB1AC185091270E556701524B88E0EC1A55918454744238C8384109279669289C6CCCF7BEF769F2A1775EA9C3A7DFBCE642148C8343CE6F6BDDDA74FD7A9FAEAABAFCE00B78E5BC7ADE3D601E091471EF99F8D75FAF4C31F9AF7A678F2E9071FFCD9F58383EF32338808440455BDE920AA8A3C4D482961776FEFC7AFBDF6DA990FA501BEFEAD6FEB57BFF14DBCF4EB17C1CC488921A250551011123314C0300CE53C819840507CF6D469ECDE7E279EFDC1F771717F9F3E2C0618E2C934097EF2EC8F70B45E43258339010044A558AABC1711540422826135601A27ECFDE6257CE5F1C7211FC063FE9FC7F1E3C7B1BFBFBF6C807FFCFD3C344F18D200610683CA0A03A20A11414A095A4DA15025A46309878787D87FF3228848EFBFFF7E25258005220C4036426616442062A82A140A024354A0A260A67A8F87A47DE63A0E51FB1D44C853061175F73233729EDEDBD9D9F9D2430F3DF4CA850B173643E0E4E7BFA03B9421F561C09018E3340144661002B20888084362AC478BFD2C8213F79DC09D77DE0122C26AB5321C01210DC94226251080340C1806B37D4A09CC0C26422ADF0DAB15982C04991920C2906C8C6118ECFAF2672FCA203F0790120360A88A2D5CCE383A3CC41D1FFF84FEFCB9E7DEFEC3EF7F77EFA207ACB362352844150313541453CE206240154A0A056C5200B2A25B958B17DFC01B6F16AB2A20B08733DB64A0E651390B880444095331AEAF4462B630220214C523AAA34044C0899173AE061615A8026948C839030A10999710DA82DD76DB1E7DE293C7EFD98A010A03BA294FEEE0000CE4140A2E6EC74C98B26104B119674809AA8A55224C53062502652D2F0F10995B4B2E61A4543D8553C2348E580D03C671C4B04A8058D8E5AC4843AA2E5E02B38C61DF6511ECECAC308D13B8788948862A415590861500C5344E383838D80E8220C638D9445A9C09380DC879AAB1344D19694810C98052B5BC96091301E37A2CABA1507563DAE1D7AA2A3825C8E1218819E338422423E501220A26BB4FD500D7425E2B1013A1AC36E1DA388289202298A609225A9C4841A09AD6FFFD9FABB8A10710999B890A186C80A8B9014CCE60767E60BE9173B6CF2A20264C63C6344D383A3AC2952B576A9C16B42AF337C71691E22536615180A98C0506A83C4701667B410F3B1FA30349D10EB8FD1A22C6B155C27868D73FF99D27F1FC2F9EEF41F0C4E7BEA8C7F4AAC51333503CA07B40404EFF8E8AE5CD0304E3B8C67ABDC6E9D3A771E6CC19FCF1DC39FC737FBF82964849AB449DF9A1D4C172447E476C4704222DA14570C8A8F79485D41AC8661840C144383A3CBA08A62F33F385CE00F77EE661DD95F7A105418918CC16635C57CCAD5AE22F678BB92C0029A669C23465BCFBEEBB78E18517F0F433CF606718CC70E5E51D44A3076C3B4480940839DBEBB44B0584042507C70C80C16C49D793EFD2C8ABD54A3FB677FBFE8B677F79A20B01C91947EBB5C5BE41B0D9AF069BA16A7557B16599A60CC07882C7A0AAE24FE7CE61351850A63273F7025FF5C40912917E76A4641EB0DA616816500D97547808404C484CD5C8C9733F10BCA35B347AEBEDCBF701C0F0C4134FE0ECD9B300802B57AF810EDE33842E71EEEE54FCAB3EA4C6A46A8D6B07B894121E7DF451FCF595574094AAF734D796024D004841C590D4206E8334A9D89BA85F139EE9A18002AC8D5C39E6B491FDF73C4D8D083DF6D8633F3D79F2E45343628BBA19C8A86AF7121A06AA315D7E67624C79C4E5CBFFC2E5CBEF6CBC8CE5E6F23211C80240CED9623F9F18FBCBB43B82A381610350140F79E7BDF7F1B7BFFC99060078FDF5D79F5AAFD7C839534F2F7BE8A931E5139AAD98D1CDDCB1B4804E0530ED05FC5CA4D164F6EB672F569FE5A9B38401421AEC8032846835B60483040F1800E0E8E8C82ABDB8EAF372B1C4146DB8333A3318C50DA651816A382707D892B24A383953D0E04D716C7FBDCEE3D44243496B5611F5E46C29B4D9A9204DC18F693C6A0618C7B1108B05B70A2E9AA7C9185F9910B981368C324B5F6551D5611D8DD25A7A42E51571C5DD6C75C597C666EA8856357531B2747851A83C092467F33A2B83A73AA919BE5416162BC1E86A52DC524BB5A8DAE2DBC7F171698E1DB9E718BECE39DBB85E5A37E6283D3E1020D354565A0B486BC189F65D077E593B563AC47C3CB7B04DCA3E9B3812E1BA3A652D636B525E002C40912BCBD3AE108DCF337A4C1D46F4311EA6AF00A5B4F192EE070AF3FF6E6C4617E243BDB10E826AC118053967CBC122809A5B8A48E309355EDB8BB77465DF734DA5DA798351E81462BBDCAF52AB42B7571623433ACB183D36497D6CA5DCDAB88648035AAE16ABE8ABD58DCB6236180816716F58E2E5EE3554CFB80B09FFBD0DC701103590A462F0B28A202A2BA88BBAA4C69A2600F6A6B1DA7CAB43F800547D444D0A0B0056C344A4726B8F53C79118AB8E09764ECDB833A0B5DABE8DEFD830959A3FE7F65CAF0A3740D1E72CD28589EB0608186578D579401B5072AEC81C413084BDE5F3229850909C620C6A70BFAC33600A61E7B54024535D162D39BD011F350F8DF8A53A0360ED2ACD180E3171713CABA484B061A918B73E3808218D61E601DA919308B23E59414F62A27788233E7AF7359E60E9D45654AA6C030528CC193153A1A17F5C08AE1C3FB8A50F604980428CC5552C96266775D12D7B179DE7EE86319B5E21E17A29145C67282F799E6A155905E0267F699FCB8BF1B4C30A3466DB446F95F63271B55C8F8BD7D7B82B56D78AF8A8C85BE720DABBA9485753E85C285153843CFE23658E4A6F284EAC5A2C62CA7CA5232DD73056570EE7F2F298090D9577537B617221335662B350B0AFA92B5B2B50D558F64AC3B0DB7FA3EA89AD8C8DB543E51FD1C815A8FC9E381F74E3741E505D9666AE1B551F10A054BFAB7F90C2B4D121FF669DA0811334E5066053BDB4C789980EBD9071E9AD3D677B9F013E7659795D1CBB1577C0468DAD1DDDF594B8590891AD5490C7BA54379F5C7013859A7AEB2B535CBA816D5B18A93C2186E10C8FBABA428A4A54F808D3061FE898E0DC321D801523350C407555FBD77A86539EC0E4BA5F0621558628A59A0C001158A42375CB8031FDF514B7145244957E0BA4129F9E707918CDB2422CC12348CD3BC2D4E55A6C64898ED5A9F50D9A7244A138411554AAB109AE09CD0A2CD709FB9A81E68289872EA1176BB4E15855854298CF33D210A513AD9DE036913E7DF5DC5B44A3445C63CF3B3B3A93A13ACAAC8A787B8C5D1FBBCEC56BFE8EAEA3B1D22D6A922A409A17D5E60D0F8892556775CC563EA0285189FD7A6DBF5A7359AD154D551B99A5415AD422882934A609A11A06959FE6CA72A707D2CC30F3C648446E40830CEE393130BEECC549EBBF791766B340712FEA3105B52C0E7C2160C0DC2710E48C2677159DBF50DD16D7D491B2E6C50DBB86E0D58C683DAF99836C0DF465A5ABC5450AAA8034D735BD87888D0E8DB5AD3B14F1BEDF42B8751563746D42E82851AD4FFA8CD6BCD54B68EB51B6F9340F8805457848437A9B446C8DB9B82D9D52439D12547B8CD0B6DA824E01AA4D369951DEA81952EE3404AF1A89A979476E1E61D92B7765BD8D994B63559A0198196918A0D354FB78F37ABD166AA507DFC9CE33D7DDD4075A66189800173AAA7BF106662C85538FEA0A1E38B4D9B847B4A57872E04BA9EE7E21003876EC98DE75D75D3DDF8E9AF782547D83AF6B1AFA609B65081FF4CAEE8E6D0F0FD45917E76C6171EDDA355CB972C5FA02AB61C0EEEE6EA5975DA371A1035B35F725E00B8A5C21D05D51342FA436B2C40D76A2C57B698BF1E672FEE6B80AE684A3A3208B477E5FCB4C63D25D03A42B77B17DE1A208BDADD515F7F66CDF3BB4FD5E2959080B9DEA5AF2D2421B6DC6EA5A393CAB077C7302C216A779CBE9466DA91BAD2616B4BAA57197C3AB651C5A1CB3E76584CDCE519CDBE07ADEB81E6B3E4DCCC6054A7A14006929A6446ABF3A4A5BF15FD411C277E5BE70FB425BBC7F3EE279FDDC1AE0376BB3BB114480C45E5916039C3A750A0F3CF0C086E87033285208688B2FC4CD5300056EBF2D5BD056B5D7454F2E2A119567A3123D5A484BDAA9D67DCB3DE1D2A5B7F1F2CBBF3503BCFAEAAB75F59D95359183C25E1B0E10D4273F9D919DCAC0D09A218E312EA428745356D7B2A8BA34EEA6E052DA9F2D7CB432B7A06596FA426DC19819972E5D6A1EE07BF56C9757AA6D641545625AECD0D82C170C506699CABEBECA14CB6EB1F0F6ADA3ECE5ADCBE7DA5729DDD8123351717B42D75788FFBAAE68A19DEA02A59496F580F538DA6E8E80F8CEBB5B8AD42091A3132528D0480FA9A9B4CCE7E4DEB7D844BA1CABD1E515D7BA172995C57243E49CC1DE2AD35E59EE769306C57A98436FDB22D7840ADB75C5B38A51BB7D01EE6AB51FE029A900680D8D99D21B812BF61728547FCE2B50363AA85A1BDEAFF38E754AB61B25179166AE13CC35866680D8F020971C0BC672AAED399EED06E9D86C29565A13B58C427D1AE2F88C486B990AB9A250C152A711A8F8E77EFB9B7B1EB36DD28CFB90624D135FBD2B86E24AC44DC61C002D151773CB037DDF708EB2EE7AACA187EFFD3DF46289EF13B03DBEFDB8DA001D48BE7F90FA0BB8FA1F56AB1DCF1120D5A205F6F38C5961282FAD85069BF81B5A4A519B23E6057D7E618BCA46CF2ED4E52210EA9BAA5E05465DA1DF55425D58F80688AE0D576A53D5823773552ADC62EF9BAF4522F474CEF9872080325570EAC945E8B9EB267DD3286FCD527ADC29E2282F5AC60EDB0AFA81FB9D5DADBFCF654B5E2FCEC60669C3922634C631B2888EE3F4BD0A7FBBBBBBF700F81A804F31711215B6CA97130012DBF4CFD474AF6D7F58F87C33F6BC4D006AFD0EB39C443D34504CFF57C39FCC3ED77322CA22F21633FFEAEAD5AB6FD1EEEE2E0E0E0E70EAD4A9E1FAF5EBC9778A8908854628A96A775E56860A0DA57E3F95D24C0CA59BFC771D9D71FD78AE14D51340396C1228BF6BF87F4E5A304DDD33524AEA5B7C8848F7F6F6A6F3E7CFE7DDDD5D1BF1EEBBEFC647EDF828BEF3ADE3D671EBB8756C1CFF05BF21B9CD4C9FE30A0000000049454E44AE426082','folder-black.png','folder-black','image/png',0);
INSERT INTO "kexi__blobs" VALUES(3,X'89504E470D0A1A0A0000000D4948445200000040000000400806000000AA6971DE000000017352474200AECE1CE900000006624B474400FF00FF00FFA0BDA79300000009704859730000375D0000375D011980465D0000000774494D4507DA0B0D172E29AA17589000000F314944415478DAED5B7B7014D59AFF9DEE9E676626F348484061632098758D052C058B1528D482C5B02A55945C37D49580BAFEB196DCBA4BA9EB0DD1D5DA95BB68B9053EB6F682AC52AC6679B8200F2D71BD370425B040B9481E4216622421092199CCAB7BBAFB9CFDC339539D9E9E64023155EB72AABABAA7FBCCF4F97EE77BFCBEEF9C016EB55BED56FBFFDCC844BD489224689A2602B01B0E21F5980248F2C36EB7EBC964F2E705000069CB962D7F535A5ABA2E180C16F9FD7E8FC7E311DD6E3762B198DED7D7178D44223DC78F1FFFE7DADADAB700A81332A80904204F92A467962E5D7A9BA669843106C61800C0E57289858585F99224F92E5FBEFC2C80F700847F6E003829A51E008410024232954F1004D2D3D39307C0F97304C05E5E5EEE120421BB3D128250289497F20F13D2847100D056585828B8DDEE51010885420EAEF6568D3186FCFC7CC76800545656A2B0B0500060BBD949BC1900BC6FBDF5D6C68B172FB6CC983163493C1EB78DF29EC96EB71B94D2AC87AEEB983469920860EA4820343636DA67CE9CF9E72D2D2D6DEFBCF3CE6F0078273A7C16EEDAB5EBD0D0D0104B2412B4ADAD8DAE5AB5AA0680D3E974F23E2280BCAAAAAAAA83070FEEECECEC8CC462311A8BC5D848872CCBB4BEBE3EFAE28B2FEE7BECB1C75601F0A77E0B2B57AE0400E78A152B9E3A7EFC384B2693B4B3B393D5D5D51D04503051C24FD9B76FDFE94422C1C2E1300B87C32C1A8DD2AEAE2EB661C386DF00C8073069F3E6CD1BDBDADABA755D67B22CB3582CC6A2D1E8B0C32C3CBF178FC799AAAAACBDBD9D1D3972A4F7D5575FFD3B004500F29F7FFEF9972F5CB8C062B1181D1A1A62914884F5F7F7B3D75F7FFDBF004C19C9C78C070F28DAB367CFA70F3EF8E02C4551867972411098AEEBA4B1B1F1ABB2B2B23F2E2D2D0D288AC21863C4D88F5F33C6322281F11EF715A228B2783C4EAE5EBD3A78F9F2E5B63973E6CCF77ABD8C524A8CFD0060EBD6AD67EAEAEA1E04D09BAB40E21884CFDFB56B57FDF2E5CBEF9565398B1327282D2D9DEAF57A5DAAAA0200313B3D1EFFADC2A0F1396F9452220802028180B3A4A4E4764992C0181B263C3FCF9E3D7BB22CCBB34E9E3C790080329E0038366CD8F0CADAB56B572B8A92319BC6B39597B79AE9915A36D028A5C33E1BC1A2948210828A8A8AE9DF7DF79DD0DEDE7E0C803E5E51207FE1C285BFD6348D716FCDCFE6C1F0819A076725849580E6BEC6CF56EF304791828202B664C9920D295F346E1A20C4E3F192850B1756381C8E0C0138B3336B84F19A3B275DD7118FC771FDFA75F4F6F6626060008AA280520A5114D3FDAC0032ABBDF91E0074777793975E7A69676F6FEF81547275F34E30A5DA53162D5A54BF63C78E4A1EEAB8D09224650060143C1C0E6360600076BB1D0E8723C3240821E919555515BAAEC3E3F1C0E7F341D7F5AC4098FD452291C033CF3CF3E5E1C3877F4908B9329AA98DD509463B3A3A1ADADBDB973DF4D04321AECA7C3082206400A0EB3A7A7A7AA0AA2A3C1E0F4451CC06EEB0EFD9ED764422115CBB760D5EAF775864C80602A5149B366D3AFFE1871FAE06F0FD4F11050060F0C2850BA7FC7E7FF59C39736CDCF118431857614A29C2E1306C361B44514C3F371FE688C085B3D96C70B95CE8EFEF87CD6683CD66B3340B0EFE810307222FBFFCF24A00E701B05C051A2B8FA600A2A5A5A576A313E202E8BA0E4110A0AA2AE2F1388CA4C4CA3F70E1ADCE1CC4BCBC3CC4E3F1B466647388C5C5C5760083A931E2A7D200E7134F3CB1A9A6A666B6D1368D5AA0284A5A78F30C9B494E3633303FE7A0DA6C36CB30A8EB3A8A8A8AA4BEBEBEE4B973E77E9F4BF8BBD164E8F6254B96AC5655D57226344D43381C4E87A56C332F08C2B083DFCBA625FC1C8D462DDF9B3203565555F557000A7FAA6C509A376F5ECDDCB973054DD3AC0680DEDE5E88A298E6094610781F411018575B455160ACFD59998C11044A2962B1588653E48C71D6AC59DEFBEFBFBF7A2C9A3D1600DC4B972E5D4908614672C307C21D9E71B09C2871E11863E8E9E9A1DBB76F3FF4C8238FAC9F39736655595959D58A152B7EF5FEFBEF1F191C1CA446476AE61700A0AA2A92C9644624A094C2EBF5B2458B165503C8BBE164C8E97442966521058E98EA230198BD7FFFFE86F2F2F2F4E0B80A8BA298F6D65C958D2ACE79426363E37FAF5EBDFA59006701C40CB62AA6063D67C78E1D6F2F5EBCF82E0EA29139F2CF8490344730B3C5B367CFE2D1471FAD04F04DEAF7293FACAACD5644C8FEC61B6FFC754545451563CCEB743AFD6EB7DBE3F7FB272B8A4244512466018D1A607CC6AF4551C4B163C74EAD59B3E617003A46F0D402803FDAB66DDBDE071E7860B699061B29AFCFE7CB0026D59FC9B2AC534ABB1863D1442211D6346DA8A9A9E9E073CF3DF72F667668150603A2286E0A0402364208E1C22412090882004A695A68FE724DD3D2F72C581EEBEAEA8AAD59B3E62900974789D114C0E5279F7C725D4343C3F1A953A7BA745D275651435114CBB048292592244994D269BAAEC3E57201002B2828A804B01B40CF683E20E07038EC8C316246DE2A31E1AA9A8D42534A497D7DFD765114DB7224284C1084D63D7BF6ECE01A6A953A7355B61A8F394701401289441E8080F9B7AC0028B6D96C695533DAD948D959B623168B61EBD6ADBB755D5772665B942A5BB66CF9F7643299B56EA0699A25005675465DD7218A22DC6E7720836358797BBBDD3E2CD5B518E0B067A9E24746764608C1D5AB57E3007E180B3D4DF5EDE8EEEE56B2016B95245901615C9AF3FBFDDE5CC2A0C8E3B6F165E6D86ED486442291610EFC733299546F70994B55555537E60C6601AD1222B386F2B1A7CC51CC0500850B64FC41B3C735A26FB4473371F1FBFDF96389CB86E60906836EF3FB47CA06CD1A6A04291E8F23994CC67301E0BADBED86A669C30018A95AC3898955C526180CA2BABAFA7E616CE55AA1A6A666492010C8AAE23CDD3602641EAFF1AC280AAE5FBF1ECE0900A34D1B9D8951AD8CD74EA7336BB9CAE170B0EAEAEAF59452AFDFEF1F55F294D09EEAEAEA67D98F2D83101967DA2CA8957F00808181010DC0502ED92003B044108429BC4C25CB32AFFCA4CBDCC64CCF6EB74355D58C82080F830505058573E7CEB57DF4D147FF395ABA2ACBB2F4C1071FFCFD3DF7DCF317945262359BC964124EA713A22866787C002C1A8D929E9E1E5CB97205172F5E44737333BEFAEAABA64B972E6D03208FC604855446350D800740FEA4499382454545955BB76E5DEB743AD3ECCE789665395D0FE0F7381B942489251209F2C30F3FBCF9F0C30FFF6D28144AF6F7F70F8B0A858585A4AFAFCF7EE8D0A17FB8F3CE3B7F4D0861BAAE1363019603108D4651545494E19C755D477777379E7EFAE96DBDBDBD5FABAADA9F9AF508804E42481F638C8EB5264852C76D9B366D3ABB78F1E220006206201E8FA7675F92A48C9C401445A6EB3AA1945EE8EAEAFAC765CB961D0030C035FFF0E1C30F9594943CEF743ACB08218C524A28A5E91AA171A6354D83CFE783B9420D801D3A74A8B7AEAE6E9E21F4B29BAD08F11FB9B67BF7EE2FEEBBEFBE554612C2CF797979E8ECEC4430184CDB20A7CEA93E441445489234A3A4A4E4779D9D9DBF638C4553AB3F1E4551C01863A9FEC498451A1D72241241717131784ACEFD11FF5E6363E3D1D4CA504E95A1B17866F9F4E9D33BBEFDF6DBAC71D9E3F14096E50CD668B2530200B1580CB22C7B1445F1C462312E2431C7714248BA989A4C26D359A031CCF1735B5B1B3EFFFCF3F7735D151A2B000CC0C9C3870F9FB1D96CCC6A974730181C962899BDB3398A58710A731CE785146E6AD9F621489284CF3EFBAC8931767A2CAC73AC25B1F0DEBD7B7FDBD6D646AC96BB28A508854296591A2F9A5A698555AE61E61C9AA661F2E4C9594BE4CDCDCDA8AFAFFFED58B7D68C15001DC08973E7CEF58CB4B8E972B9C0F3092338C6D83D1A10FC99AAAAD0340DC5C5C5E999CEC8E80401CDCDCD5792C9E4E9B11444C7541576BBDD5055D5B960C1825FAD5BB7AE8AAFF0980F1E09EC763B344D437F7F3F3C1E8F6552956DB98B1FAAAAC2E57265AC105991A0A2A222DFF7DF7F3FD4D1D1D1E476BB354EE6C60D005555ED1515158F6FDCB87173281462E4C766293CBF76381C080402E8EBEB43329984DD6ECFBA0A6CAE2EDB6C36F8FD7EA496C387990377C2463371B95C6CE6CC998BBFF9E69B4B57AF5E3D9FAB26E404407E7EBE545E5EBEACB6B6F6DFA64C99C28C858A6CC21B0B193E9F0F7EBF3F4D948C24C9D8571445381C0E78BD5EF09A8439EF303B4DC37D120804D8DD77DFFD484B4BCB094DD32EC9B24C732139B9B4C0A79F7EDAE3F3F924C61831D7FC8CD71C04E3753653318398CD4CAC2AC0DCA96A9A96E609A9E70C4064FEFCF97700B83E5E4E906CDFBEFD822008245BEDDE4AA5475273A330D92A4F56F7B8A03C941ACBE87C589F7CF2C9A5F176824A7373F399BCBCBCBF9C356B96646502233944AB25B46C0066CBF9CD2A6F95250A82C01A1A1AA2B5B5B5BF00D09E0B1F10C740827A9B9A9A5A4B4B4B57969595112B100441405353130020140A314E69AD4018A9A069C5045B5B5BD1DCDC8C69D3A66508CE853F75EA94F6C20B2FFC52D7F5DF33C6F4718D0229AF7AE9C4891397A64F9FFE706969697A0313E7EB6FBFFD76DB2BAFBCB276EFDEBD7F5055757A5151516128141A71E3859509A5C064822090F3E7CFE3DD77DF3D575B5B5BBB7FFFFEEDFDFDFDF3EFBDF7DE00CF330CC2EBB5B5B54F85C3E1FF608CE54C856F64BBBCCBE170ACA8ABABDBB17CF9723B630C914884BCF9E69B8D1F7FFCF19384908BA95D5C0582202CAEAAAA5A535959B970C1820579C5C5C58C97DBCD8ECFB07EC8BABBBBC9D75F7F1D397AF4E8B1A3478F7E00E00F00AEA53471466565E5BFBEF6DA6B7F969F9FCF1863E48B2FBE88D7D6D6AE4D24129F30C6129880E600707F4D4D4DC7CE9D3BD9DCB973EB014C2599F450C08FDB58CB7D3EDF3F353434B0969616D6DADACA5A5B5B2DAFBFFCF24B160A853603284F7D57B0F8CDDBEFB8E38E7DBB77EF66EBD7AF6F07B0581004C78D0872C37F982084488CB1DB00DC05E02421A47F943D3977BDF7DE7BE7172C5890B5D64F08415353131E7FFCF13F01D03CCAB8834EA7739E2CCB2DA9DC5FBB11396E78A735634C4BADF375586D7AB068314551B4147059418844224A6AE17434A7DC2FCBF2919B55650113D79276BB5DCEF667099ED40C0E0EC691C3F6B6F16A130980DAD7D7171145D19233F09C7F6868283691004CE43F4612A74E9DFA9FE9D3A74F0E0683C8CFCF4F6FA8482693181C1C445F5F1FCE9C39F39DB972FB53B609FBD7182144608C4D01F0A7849012A7D31902E04CD5FE154551AEA596CFCF0882D045B32D39FF5F05C0F44EC1506D36165EE91817516FB55BED56BBB9F6BFD861F1268BFE47BF0000000049454E44AE426082','application-x-executable.png','application-x-executable','image/png',0);
CREATE TABLE cars (id INTEGER PRIMARY KEY, model Text(200));
INSERT INTO "cars" VALUES(1,'Fiat');
INSERT INTO "cars" VALUES(2,'Syrena');
INSERT INTO "cars" VALUES(3,'Chrysler');
INSERT INTO "cars" VALUES(4,'Volvo');
INSERT INTO "cars" VALUES(5,'BMW');
CREATE TABLE ownership (id INTEGER PRIMARY KEY, owner Integer, car Integer, since Integer);
INSERT INTO "ownership" VALUES(1,1,1,2004);
INSERT INTO "ownership" VALUES(2,2,2,1982);
INSERT INTO "ownership" VALUES(3,3,3,2002);
INSERT INTO "ownership" VALUES(4,4,4,2005);
INSERT INTO "ownership" VALUES(5,10,4,2006);
INSERT INTO "ownership" VALUES(6,4,1,2003);
INSERT INTO "ownership" VALUES(7,12,3,1999);
CREATE TABLE kexi__userdata (d_user Text NOT NULL, o_id Integer UNSIGNED NOT NULL, d_sub_id Text NOT NULL, d_data CLOB);
INSERT INTO "kexi__userdata" VALUES('',104,'columnWidths','120,120,120,120,120');
INSERT INTO "kexi__userdata" VALUES('',1,'columnWidths','120,120,120,120');
INSERT INTO "kexi__userdata" VALUES('',103,'columnWidths','120,120,120,120');
INSERT INTO "kexi__userdata" VALUES('',2,'columnWidths','120,120');
INSERT INTO "kexi__userdata" VALUES('',107,'columnWidths','120,120,120');
COMMIT;

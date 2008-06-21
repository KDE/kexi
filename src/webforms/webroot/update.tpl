{{>beforecontent}}
    <h3>Edit a row in table: {{TABLENAME}}</h3>
    <br/>
    {{#SUCCESS}}<h4 style="color: green;">{{MESSAGE}}</h4>{{/SUCCESS}}
    {{#ERROR}}<h4 style="color: red;">{{MESSAGE}}</h4>{{/ERROR}}
    
    {{#FORM}}
        <form action="/update/{{TABLENAME}}/{{PKEY_NAME}}/{{PKEY_VALUE}}" method="POST">
        <table border="0" width="100%">
            {{FORMDATA}}	
        </table>
        <input type="hidden" name="dataSent" value="true"/>
        <input type="hidden" name="tableFields" value="{{TABLEFIELDS}}"/>
        <input type="submit"/>
        <input type="reset"/>
        </form>
    {{/FORM}}
{{>aftercontent}}

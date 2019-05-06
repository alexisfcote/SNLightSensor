{%- extends 'full.tpl' -%}

{%- block html_head -%}
    {{super()}}
    <style type="text/css">
    .container {
    text-align:center;
    }
    </style>
{%- endblock html_head -%}

{% block input_group -%}
{% endblock input_group %}

{% block stream_stderr -%}
{%- endblock stream_stderr %}

{% block output_area_prompt %}
{% endblock output_area_prompt %}
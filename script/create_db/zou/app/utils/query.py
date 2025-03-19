import math
import orjson as json
import sqlalchemy.orm as orm

from zou.app import app
from zou.app.utils import fields, string
from sqlalchemy import func
from sqlalchemy.inspection import inspect


def get_query_criterions_from_request(request):
    """
    Turn request parameters into a dict where keys are attributes to filter and
    values are values to filter.
    """
    criterions = {}
    for key, value in request.args.items():
        if key not in ["page"]:
            criterions[key] = value
    return criterions


def apply_criterions_to_db_query(model, db_query, criterions):
    """
    Apply criterions given in HTTP request to the sqlachemy db query object.
    """

    many_join_filter = []
    in_filter = []
    name_filter = []
    filters = {}

    column_names = inspect(model).all_orm_descriptors.keys()
    for key, value in criterions.items():
        if key not in ["page", "relations"] and key in column_names:
            field_key = getattr(model, key)

            is_many_to_many_field = hasattr(
                field_key, "property"
            ) and isinstance(
                field_key.property, orm.properties.RelationshipProperty
            )
            value_is_list = (
                hasattr(value, "__len__")
                and len(value) > 0
                and value[0] == "["
            )

            if key == "name" and field_key is not None:
                name_filter.append(value)

            elif is_many_to_many_field:
                many_join_filter.append((key, value))

            elif value_is_list:
                value_array = json.loads(value)
                in_filter.append(
                    field_key.in_(
                        [cast_value(value, field_key) for value in value_array]
                    )
                )
            else:
                filters[key] = cast_value(value, field_key)

        if filters:
            db_query = db_query.filter_by(**filters)

        for value in name_filter:
            db_query = db_query.filter(model.name.ilike(value))

        for id_filter in in_filter:
            db_query = db_query.filter(id_filter)

        for key, value in many_join_filter:
            db_query = db_query.filter(getattr(model, key).any(id=value))

    return db_query


def get_paginated_results(query, page, limit=None, relations=False):
    """
    Apply pagination to the query object.
    """
    if page < 1:
        entries = query.all()
        return fields.serialize_models(entries, relations=relations)
    else:
        limit = limit or app.config["NB_RECORDS_PER_PAGE"]
        total = query.count()
        offset = (page - 1) * limit

        nb_pages = int(math.ceil(total / float(limit)))
        query = query.limit(limit)
        query = query.offset(offset)

        if total < offset:
            result = {
                "data": [],
                "total": 0,
                "nb_pages": nb_pages,
                "limit": limit,
                "offset": offset,
                "page": page,
            }
        else:
            models = fields.serialize_models(query.all(), relations=relations)
            result = {
                "data": models,
                "total": total,
                "nb_pages": nb_pages,
                "limit": limit,
                "offset": offset,
                "page": page,
            }
        return result


def get_cursor_results(
    model,
    query,
    cursor_created_at,
    limit=None,
    relations=False,
):
    """ """
    limit = limit or app.config["NB_RECORDS_PER_PAGE"]
    total = query.count()
    query = (
        query.filter(model.created_at > cursor_created_at)
        .order_by(model.created_at, model.updated_at, model.id)
        .limit(limit)
    )
    models = fields.serialize_models(
        query.all(), relations=relations, milliseconds=True
    )
    result = {
        "data": models,
        "total": total,
        "limit": limit,
    }
    return result


def apply_sort_by(model, query, sort_by):
    """
    Apply an order by clause to a sqlalchemy query from a string parameter.
    """
    if sort_by in model.__table__.columns.keys():
        sort_field = model.__table__.columns[sort_by]
        if sort_by in ["created_at", "updated_at"]:
            sort_field = sort_field.desc()
    else:
        sort_field = model.updated_at.desc()
    return query.order_by(sort_field)


def cast_value(value, field_key):
    if field_key.type.python_type is bool:
        return string.strtobool(value)
    else:
        return func.cast(value, field_key.type)

from flask_restful import Api
from zou.app.utils.flask import output_json


def configure_api_from_blueprint(blueprint, route_tuples, decorators=None):
    """
    Creates a Flask Restful api object based on information from given
    blueprint. API is configured to return JSON objects.

    Each blueprint is describe by a list of tuple. Each tuple is composed of a
    route and the related resource (controller).
    """

    api = Api(blueprint, catch_all_404s=True, decorators=decorators)

    api.representations = {
        "application/json": output_json,
    }

    for route_tuple in route_tuples:
        (path, resource) = route_tuple
        api.add_resource(resource, path)

    return api

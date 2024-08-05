import superagent from 'superagent';

export default {
  get(page: number, page_size: number) {
    return superagent.get('/v1/task').query({ page, page_size });
  },
};
